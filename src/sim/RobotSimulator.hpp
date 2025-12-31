#pragma once

#include "../model/Robot.hpp"
#include <mutex>
#include <cmath>
#include <chrono>
#include <functional>

namespace tsim::sim
{
    class RobotSimulator
    {
    public:
        using on_status_change_fn = std::function<void(const model::RobotStatus&)>;

        RobotSimulator()
        {
            m_status.bInHome = 1;
            m_status.bEnabled = 1;
            m_status.bBrakeTestOk = 1;
            m_status.bMasteringOk = 1;
            m_status.bInExt = 1;
            m_status.nAreaFree_Robot = 0xFF;
        }

        void update_control(const model::RobotControl& ctrl)
        {
            std::lock_guard lock(m_mutex);
            m_control = ctrl;
            
            if (m_control.bReset) {
                m_status.bError = 0;
                m_status.nErrorCode = 0;
            }

            // Start Job Logic
            if (m_control.nJobId != 0 && m_control.nJobId != m_status.nJobIdFeedback && !m_isExecuting) {
                if (m_control.bMoveEnable && !m_status.bError) {
                    m_isExecuting = true;
                    m_jobProgress = 0.0f;
                    m_status.bInHome = 0;
                }
            }
        }

        void set_manual_error(bool active)
        {
            std::lock_guard lock(m_mutex);
            m_errorManual = active;
            m_status.bError = active ? 1 : 0;
            if (active) m_isExecuting = false;
        }

        void toggle_area_manual(int index, bool free)
        {
            std::lock_guard lock(m_mutex);
            if (free)
                m_status.nAreaFree_Robot |= (1 << index);
            else
                m_status.nAreaFree_Robot &= ~(1 << index);
        }

        model::RobotStatus get_status() const
        {
            std::lock_guard lock(m_mutex);
            return m_status;
        }

        model::RobotControl get_control() const
        {
            std::lock_guard lock(m_mutex);
            return m_control;
        }

        void step(float dt)
        {
            bool changed = false;
            {
                std::lock_guard lock(m_mutex);
                
                // Mirror the part type immediately
                if (m_status.nPartTypeMirrored != m_control.nPartType) {
                    m_status.nPartTypeMirrored = m_control.nPartType;
                    changed = true;
                }

                if (m_isExecuting) {
                    if (m_control.bMoveEnable && !m_status.bError) {
                        m_jobProgress += dt;
                        
                        // Simulation Timeline (Total 4s)
                        if (m_jobProgress < 1.5f) {
                            // Phase 1: Moving to Area
                            if (m_status.bInMotion == 0) { m_status.bInMotion = 1; changed = true; }
                        } 
                        else if (m_jobProgress < 2.5f) {
                            // Phase 2: Working in Area (Occupied)
                            if (m_status.bInMotion == 1) { m_status.bInMotion = 0; changed = true; }
                            
                            int area_idx = m_control.nJobId % 8;
                            // Automatically occupy area during work phase
                            uint8_t mask = ~(1 << area_idx);
                            if ((m_status.nAreaFree_Robot & (1 << area_idx)) != 0) {
                                m_status.nAreaFree_Robot &= mask;
                                changed = true;
                            }
                        }
                        else if (m_jobProgress < 4.0f) {
                            // Phase 3: Moving back to Home
                            if (m_status.bInMotion == 0) { m_status.bInMotion = 1; changed = true; }
                            
                            // Free area after work
                            int area_idx = m_control.nJobId % 8;
                            if ((m_status.nAreaFree_Robot & (1 << area_idx)) == 0) {
                                m_status.nAreaFree_Robot |= (1 << area_idx);
                                changed = true;
                            }
                        }
                        else {
                            // Phase 4: Job Complete
                            m_status.bInMotion = 0;
                            m_status.bInHome = 1;
                            m_status.nJobIdFeedback = m_control.nJobId; // Update Feedback here
                            m_isExecuting = false;
                            changed = true;
                        }
                    } else {
                        if (m_status.bInMotion == 1) { m_status.bInMotion = 0; changed = true; }
                    }
                } else {
                    if (m_control.nJobId == 0 && m_status.bInHome == 0) {
                        m_status.bInHome = 1;
                        m_status.nJobIdFeedback = 0;
                        changed = true;
                    }
                }
            }
            if (changed && on_status_change) on_status_change(get_status());
        }

        on_status_change_fn on_status_change;

    private:
        mutable std::mutex m_mutex;
        model::RobotControl m_control{};
        model::RobotStatus m_status{};
        
        bool m_isExecuting = false;
        float m_jobProgress = 0.0f;
        bool m_errorManual = false;
    };
}
