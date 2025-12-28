#pragma once
#include <coroutine>
#include <deque>
#include <optional>
#include <mutex>
#include <utility>

namespace tlink {

/**
 * @brief An asynchronous channel (queue) for passing data from producer (driver) to consumer (coroutine).
 * This allows a "pull-based" subscription API.
 */
template<typename T>
class AsyncChannel {
public:
    AsyncChannel() = default;

    // Delete copy/move to keep things simple (intended to be held via shared_ptr)
    AsyncChannel(const AsyncChannel&) = delete;
    AsyncChannel& operator=(const AsyncChannel&) = delete;

    /**
     * @brief Pushes a value into the channel. Resumes a waiting consumer if one exists.
     * Called by the Driver (producer).
     */
    void push(T value) {
        std::unique_lock lock(m_mutex);
        m_queue.push_back(std::move(value));

        if (m_waiting_coroutine) {
            auto handle = m_waiting_coroutine;
            m_waiting_coroutine = nullptr;
            lock.unlock(); // Unlock before resuming to avoid deadlock
            handle.resume();
        }
    }

    /**
     * @brief Awaitable object for fetching the next value.
     */
    struct NextAwaiter {
        AsyncChannel* channel;
        T result;

        bool await_ready() {
            std::lock_guard lock(channel->m_mutex);
            return !channel->m_queue.empty();
        }

        void await_suspend(std::coroutine_handle<> h) {
            std::lock_guard lock(channel->m_mutex);
            // In a robust impl, check if queue became non-empty again or if another waiter exists
            channel->m_waiting_coroutine = h;
        }

        T await_resume() {
            std::lock_guard lock(channel->m_mutex);
            T val = std::move(channel->m_queue.front());
            channel->m_queue.pop_front();
            return val;
        }
    };

    /**
     * @brief Co_await this to get the next value. Suspends if empty.
     */
    NextAwaiter next() {
        return NextAwaiter{this};
    }

private:
    std::mutex m_mutex;
    std::deque<T> m_queue;
    std::coroutine_handle<> m_waiting_coroutine = nullptr;
};

} // namespace tlink
