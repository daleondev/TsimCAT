#pragma once

#include "utils.hpp"

#include <condition_variable>
#include <coroutine>
#include <deque>

namespace tlink::coro
{
    class IExecutor
    {
      public:
        virtual ~IExecutor() = default;
        virtual auto run() -> void = 0;
        virtual auto stop() -> void = 0;
        virtual auto schedule(std::coroutine_handle<> handle) -> void = 0;
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

        auto run() -> void override
        {
            while (m_running) {
                std::unique_lock lock(m_mutex);

                while (!m_queue.empty()) {
                    auto handle{ utils::pop(m_queue) };

                    lock.unlock();
                    if (handle && !handle.done()) {
                        handle.resume();
                    }
                    lock.lock();
                }

                m_cv.wait(lock, [this] { return !m_queue.empty() || !m_running; });
            }
        }

        auto stop() -> void override
        {
            m_running = false;
            m_cv.notify_all();
        }

        auto schedule(std::coroutine_handle<> handle) -> void override
        {
            utils::push(m_queue, handle, m_mutex);
            m_cv.notify_one();
        }

        auto getLifeToken() -> std::weak_ptr<void> override { return m_lifeToken; }

      private:
        std::atomic<bool> m_running{ true };
        std::mutex m_mutex{};
        std::condition_variable m_cv{};
        std::deque<std::coroutine_handle<>> m_queue{};
        std::shared_ptr<bool> m_lifeToken{ std::make_shared<bool>(true) };
    };
}