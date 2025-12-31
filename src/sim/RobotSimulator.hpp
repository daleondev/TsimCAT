#pragma once

#include "../model/Robot.hpp"
#include <mutex>
#include <algorithm>
#include <cmath>

namespace tsim::sim
{
    class RobotSimulator
    {
    public:
        RobotSimulator()
        {
            m_status.fActualJointAngles.fill(0.0f);
            m_control.fTargetJointAngles.fill(0.0f);
        }

        void update_control(const model::RobotControl& ctrl)
        {
            std::lock_guard lock(m_mutex);
            m_control = ctrl;
            if (m_control.bReset) {
                m_status.bError = false;
                m_status.nErrorCode = 0;
            }
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
            std::lock_guard lock(m_mutex);

            m_status.bEnabled = m_control.bEnable;

            if (m_status.bEnabled && !m_status.bError) {
                for (size_t i = 0; i < 6; ++i) {
                    float target = m_control.fTargetJointAngles[i];
                    float actual = m_status.fActualJointAngles[i];
                    float diff = target - actual;
                    
                    // Simple speed limit: 90 degrees per second
                    float max_step = 90.0f * dt;
                    
                    if (std::abs(diff) < max_step) {
                        m_status.fActualJointAngles[i] = target;
                    } else {
                        m_status.fActualJointAngles[i] += (diff > 0 ? max_step : -max_step);
                    }
                }
            }
        }

    private:
        mutable std::mutex m_mutex;
        model::RobotControl m_control{};
        model::RobotStatus m_status{};
    };
}
