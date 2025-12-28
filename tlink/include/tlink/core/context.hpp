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
        while (!m_readyQueue.empty() || !m_timers.empty()) {
            // 1. Process all ready tasks
            while (!m_readyQueue.empty()) {
                auto handle = m_readyQueue.front();
                m_readyQueue.pop_front();
                if (handle && !handle.done()) {
                    handle.resume();
                }
            }

            // 2. Check timers and move expired ones to ready queue
            if (!m_timers.empty()) {
                auto now = std::chrono::steady_clock::now();
                auto it = m_timers.begin();
                while (it != m_timers.end()) {
                    if (now >= it->m_expiry) {
                        m_readyQueue.push_back(it->m_handle);
                        it = m_timers.erase(it);
                    } else {
                        ++it;
                    }
                }

                if (m_readyQueue.empty() && !m_timers.empty()) {
                    // Small sleep to prevent busy looping if only waiting for timers
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    }

    auto schedule(std::coroutine_handle<> handle) -> void {
        m_readyQueue.push_back(handle);
    }

    auto scheduleTimer(std::chrono::steady_clock::time_point expiry, std::coroutine_handle<> handle) -> void {
        m_timers.push_back({expiry, handle});
    }

private:
    struct TimerEntry {
        std::chrono::steady_clock::time_point m_expiry;
        std::coroutine_handle<> m_handle;
    };

    std::deque<std::coroutine_handle<>> m_readyQueue;
    std::vector<TimerEntry> m_timers;
};

} // namespace tlink
