#pragma once
#include <deque>
#include <vector>
#include <chrono>
#include <coroutine>
#include <thread>

namespace tlink {

/**
 * @brief The event loop and scheduler for TLink.
 * Manages the execution of coroutines and timers.
 */
class Context {
public:
    auto run() -> void {
        while (!m_ready_queue.empty() || !m_timers.empty()) {
            // 1. Process all ready tasks
            while (!m_ready_queue.empty()) {
                auto handle = m_ready_queue.front();
                m_ready_queue.pop_front();
                if (handle && !handle.done()) {
                    handle.resume();
                }
            }

            // 2. Check timers and move expired ones to ready queue
            if (!m_timers.empty()) {
                auto now = std::chrono::steady_clock::now();
                auto it = m_timers.begin();
                while (it != m_timers.end()) {
                    if (now >= it->expiry) {
                        m_ready_queue.push_back(it->handle);
                        it = m_timers.erase(it);
                    } else {
                        ++it;
                    }
                }

                if (m_ready_queue.empty() && !m_timers.empty()) {
                    // Small sleep to prevent busy looping if only waiting for timers
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    }

    auto schedule(std::coroutine_handle<> handle) -> void {
        m_ready_queue.push_back(handle);
    }

    auto schedule_timer(std::chrono::steady_clock::time_point expiry, std::coroutine_handle<> handle) -> void {
        m_timers.push_back({expiry, handle});
    }

private:
    struct TimerEntry {
        std::chrono::steady_clock::time_point expiry;
        std::coroutine_handle<> handle;
    };

    std::deque<std::coroutine_handle<>> m_ready_queue;
    std::vector<TimerEntry> m_timers;
};

} // namespace tlink
