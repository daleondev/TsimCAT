#pragma once

#include <atomic>
#include <concepts>
#include <condition_variable>
#include <coroutine>
#include <deque>
#include <mutex>

namespace tlink::coro
{
    class IExecutor
    {
      public:
        virtual ~IExecutor() = default;
        virtual void run() = 0;
        virtual void stop() = 0;
        virtual void schedule(std::coroutine_handle<> handle) = 0;
        virtual auto getLifeToken() -> std::weak_ptr<void> = 0;
    };

    template<typename T>
    concept Executor = std::is_base_of_v<IExecutor, T>;

    class Context : public IExecutor
    {
      public:
        Context() = default;
        ~Context() = default;
        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context&&) = delete;

        void run()
        {
            while (m_running) {
                std::unique_lock lock(m_mutex);

                while (!m_queue.empty()) {
                    auto handle{ std::move(m_queue.front()) };
                    m_queue.pop_front();

                    lock.unlock();
                    if (handle && !handle.done()) {
                        handle.resume();
                    }
                    lock.lock();
                }

                m_cv.wait(lock, [this] { return !m_queue.empty() || !m_running; });
            }
        }

        void stop()
        {
            m_running = false;
            m_cv.notify_all();
        }

        void schedule(std::coroutine_handle<> handle)
        {
            {
                std::lock_guard lock(m_mutex);
                m_queue.push_back(handle);
            }
            m_cv.notify_one();
        }

        auto getLifeToken() -> std::weak_ptr<void> override { return m_lifeToken; }

      private:
        std::atomic<bool> m_running{ true };
        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::deque<std::coroutine_handle<>> m_queue;
        std::shared_ptr<int> m_lifeToken{ std::make_shared<int>(0) };
    };
}