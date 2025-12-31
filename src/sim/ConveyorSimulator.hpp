#pragma once

#include "../model/Conveyor.hpp"
#include <atomic>
#include <mutex>
#include <cmath>
#include <print>
#include <iostream>

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
            
            // Logging to debug mapping issues
            std::println("Sim: Command received - Run: {}, Reverse: {}", (int)m_control.bRun, (int)m_control.bReverse);

            if (m_control.bRun != 0 && !m_errorManual) {
                m_status.bRunning = 1;
            } else if (m_actualVelocity == 0.0f) {
                m_status.bRunning = 0;
            }
        }

        void set_manual_error(bool active)
        {
            std::lock_guard lock(m_mutex);
            m_errorManual = active;
            m_status.bError = active ? 1 : 0;
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
            std::lock_guard lock(m_mutex);
            
            const float targetMaxSpeed = 0.5f; 

            if (m_control.bRun != 0 && !m_errorManual) {
                m_status.bRunning = 1;
                
                // Direction: 0 = Forward (+), 1 = Backward (-)
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
                    m_actualVelocity = 0.0f;
                    m_status.bRunning = 0;
                } else {
                    m_actualVelocity += (m_actualVelocity > 0 ? -step_size : step_size);
                }
            }
        }

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
