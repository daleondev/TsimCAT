#pragma once

#include "../model/Conveyor.hpp"
#include <atomic>
#include <mutex>
#include <cmath>
#include <functional>

namespace tsim::sim
{
    class ConveyorSimulator
    {
    public:
        using on_status_change_fn = std::function<void(const model::ConveyorStatus&)>;

        ConveyorSimulator() = default;

        void update_control(const model::ConveyorControl& ctrl)
        {
            bool changed = false;
            {
                std::lock_guard lock(m_mutex);
                m_control = ctrl;
                
                // Logic: If PLC says RUN or REVERSE, and no error, we signal Running
                if ((m_control.bRun != 0 || m_control.bReverse != 0) && !m_errorManual) {
                    if (m_status.bRunning == 0) {
                        m_status.bRunning = 1;
                        changed = true;
                    }
                } else if (m_actualVelocity == 0.0f) {
                    if (m_status.bRunning != 0) {
                        m_status.bRunning = 0;
                        changed = true;
                    }
                }
            }
            if (changed && on_status_change) {
                on_status_change(get_status());
            }
        }

        void set_manual_error(bool active)
        {
            {
                std::lock_guard lock(m_mutex);
                m_errorManual = active;
                m_status.bError = active ? 1 : 0;
            }
            if (on_status_change) {
                on_status_change(get_status());
            }
        }

        model::ConveyorStatus get_status() const
        {
            std::lock_guard lock(m_mutex);
            return m_status;
        }

        model::ConveyorControl get_control() const
        {
            std::lock_guard lock(m_mutex);
            return m_control;
        }

        float get_actual_velocity() const { std::lock_guard lock(m_mutex); return m_actualVelocity; }
        uint32_t get_item_count() const { std::lock_guard lock(m_mutex); return m_itemCount; }

        void step(float dt)
        {
            bool status_changed = false;
            {
                std::lock_guard lock(m_mutex);
                
                const float targetMaxSpeed = 0.5f; 

                if ((m_control.bRun != 0 || m_control.bReverse != 0) && !m_errorManual) {
                    m_status.bRunning = 1;
                    
                    float target = (m_control.bReverse != 0) ? -targetMaxSpeed : targetMaxSpeed;
                    float diff = target - m_actualVelocity;
                    float step_size = 2.0f * dt; 
                    
                    if (std::abs(diff) < step_size) {
                        m_actualVelocity = target;
                    } else {
                        m_actualVelocity += (diff > 0 ? step_size : -step_size);
                    }

                    m_distance_buffer += std::abs(m_actualVelocity) * dt;
                    if (m_distance_buffer >= 1.0f) { 
                        m_itemCount++;
                        m_distance_buffer -= 1.0f;
                    }
                } else {
                    float step_size = 3.0f * dt;
                    if (std::abs(m_actualVelocity) < step_size) {
                        if (m_actualVelocity != 0.0f || m_status.bRunning != 0) {
                            m_actualVelocity = 0.0f;
                            m_status.bRunning = 0;
                            status_changed = true; 
                        }
                    } else {
                        m_actualVelocity += (m_actualVelocity > 0 ? -step_size : step_size);
                    }
                }
            }
            if (status_changed && on_status_change) {
                on_status_change(get_status());
            }
        }

        on_status_change_fn on_status_change;

    private:
        mutable std::mutex m_mutex;
        model::ConveyorControl m_control{};
        model::ConveyorStatus m_status{};
        
        bool m_errorManual = false;
        float m_actualVelocity = 0.0f;
        uint32_t m_itemCount = 0;
        float m_distance_buffer = 0.0f;
    };
}