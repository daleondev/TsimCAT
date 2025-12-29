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

        if (m_waitingCoroutine) {
            auto handle = m_waitingCoroutine;
            m_waitingCoroutine = nullptr;
            lock.unlock(); // Unlock before resuming to avoid deadlock
            handle.resume();
        }
    }

    /**
     * @brief Pushes a value into the channel WITHOUT resuming the waiter.
     * Use with take_waiter() to manually schedule resumption on a specific context.
     */
    void push_silent(T value) {
        std::lock_guard lock(m_mutex);
        m_queue.push_back(std::move(value));
    }

    /**
     * @brief Extracts the waiting coroutine handle if one exists.
     */
    auto take_waiter() -> std::coroutine_handle<> {
        std::lock_guard lock(m_mutex);
        auto h = m_waitingCoroutine;
        m_waitingCoroutine = nullptr;
        return h;
    }

    /**
     * @brief Awaitable object for fetching the next value.
     */
    struct NextAwaiter {
        AsyncChannel* m_channel;
        T m_result;

        bool await_ready() {
            std::lock_guard lock(m_channel->m_mutex);
            return !m_channel->m_queue.empty();
        }

        void await_suspend(std::coroutine_handle<> h) {
            std::lock_guard lock(m_channel->m_mutex);
            // In a robust impl, check if queue became non-empty again or if another waiter exists
            m_channel->m_waitingCoroutine = h;
        }

        T await_resume() {
            std::lock_guard lock(m_channel->m_mutex);
            T val = std::move(m_channel->m_queue.front());
            m_channel->m_queue.pop_front();
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
    std::coroutine_handle<> m_waitingCoroutine = nullptr;
};

} // namespace tlink
