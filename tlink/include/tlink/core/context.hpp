#pragma once
#include <deque>
#include <vector>
#include <chrono>
#include <coroutine>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace tlink {

/**
 * @brief The event loop and scheduler for TLink.
 * Manages the execution of coroutines and timers.
 * Now thread-safe and blocking.
 */
class Context {
public:
    Context() : m_running(true) {}

    auto run() -> void {
        while (m_running) {
            std::unique_lock lock(m_mutex);
            
            // 1. Process all ready tasks
            while (!m_readyQueue.empty()) {
                auto handle = m_readyQueue.front();
                m_readyQueue.pop_front();
                
                // Unlock while executing task to allow re-entrancy/scheduling from task
                lock.unlock();
                if (handle && !handle.done()) {
                    handle.resume();
                }
                lock.lock();
            }

            if (!m_running) break;

            // 2. Check timers
            bool hasTimers = !m_timers.empty();
            std::chrono::steady_clock::time_point nextExpiry = std::chrono::steady_clock::time_point::max();
            
            if (hasTimers) {
                auto now = std::chrono::steady_clock::now();
                auto it = m_timers.begin();
                while (it != m_timers.end()) {
                    if (now >= it->m_expiry) {
                        m_readyQueue.push_back(it->m_handle);
                        it = m_timers.erase(it);
                    } else {
                        if (it->m_expiry < nextExpiry) {
                            nextExpiry = it->m_expiry;
                        }
                        ++it;
                    }
                }
            }

            // If we pushed tasks from timers, loop again immediately to process them
            if (!m_readyQueue.empty()) {
                continue;
            }

            // 3. Wait for work
            if (m_running) {
                if (hasTimers) {
                    m_cv.wait_until(lock, nextExpiry, [&]{ 
                        return !m_readyQueue.empty() || !m_running; 
                    });
                } else {
                    m_cv.wait(lock, [&]{ 
                        return !m_readyQueue.empty() || !m_running; 
                    });
                }
            }
        }
    }

    auto schedule(std::coroutine_handle<> handle) -> void {
        {
            std::lock_guard lock(m_mutex);
            m_readyQueue.push_back(handle);
        }
        m_cv.notify_one();
    }

    auto scheduleTimer(std::chrono::steady_clock::time_point expiry, std::coroutine_handle<> handle) -> void {
        {
            std::lock_guard lock(m_mutex);
            m_timers.push_back({expiry, handle});
        }
        m_cv.notify_one();
    }

    auto stop() -> void {
        {
            std::lock_guard lock(m_mutex);
            m_running = false;
        }
        m_cv.notify_all();
    }

private:
    struct TimerEntry {
        std::chrono::steady_clock::time_point m_expiry;
        std::coroutine_handle<> m_handle;
    };

    std::deque<std::coroutine_handle<>> m_readyQueue;
    std::vector<TimerEntry> m_timers;
    
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running;
};

} // namespace tlink
