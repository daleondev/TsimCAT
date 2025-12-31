#pragma once

#include "../model/Conveyor.hpp"
#include <atomic>
#include <mutex>
#include <cmath>

namespace tsim::sim
{
    class ConveyorSimulator
    {
    public:
        ConveyorSimulator() = default;

        void update_control(const model::ConveyorControl& ctrl)
        {
            std::lock_guard lock(m_mutex);
            m_control = ctrl;
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

        /**
         * Simple simulation step.
         */
        void step(float dt)
        {
            std::lock_guard lock(m_mutex);
            
            // Simulation logic
            const float targetSpeed = 0.5f; // Hardcoded default speed for simulation

            if (m_control.bRun) {
                m_status.bRunning = true;
                
                // Ramp actual velocity towards target
                float target = m_control.bReverse ? -targetSpeed : targetSpeed;
                float diff = target - m_actualVelocity;
                float step = 2.0f * dt; 
                
                if (std::abs(diff) < step) {
                    m_actualVelocity = target;
                } else {
                    m_actualVelocity += (diff > 0 ? step : -step);
                }

                // Update item count
                m_distance_buffer += std::abs(m_actualVelocity) * dt;
                if (m_distance_buffer >= 1.0f) { 
                    m_itemCount++;
                    m_distance_buffer -= 1.0f;
                }
            } else {
                // Decelerate to zero
                float step = 3.0f * dt;
                if (std::abs(m_actualVelocity) < step) {
                    m_actualVelocity = 0.0f;
                    m_status.bRunning = false;
                } else {
                    m_actualVelocity += (m_actualVelocity > 0 ? -step : step);
                }
            }
        }

    private:
        mutable std::mutex m_mutex;
        model::ConveyorControl m_control{};
        model::ConveyorStatus m_status{};
        
        // Internal state (removed from PLC communication)
        float m_actualVelocity = 0.0f;
        uint32_t m_itemCount = 0;
        float m_distance_buffer = 0.0f;
    };
}