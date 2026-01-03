#pragma once

#include "Context.hpp"

#include <exception>
#include <optional>
#include <utility>

namespace tlink::coro
{
    namespace detail
    {
        struct DetachedTaskPromise;
        template<typename T>
        struct TaskPromise;
    }

    class DetachedTask
    {
      public:
        using promise_type = detail::DetachedTaskPromise;
        using handle_type = std::coroutine_handle<promise_type>;

        DetachedTask(handle_type handle)
          : m_handle{ handle }
        {
        }
        ~DetachedTask()
        {
            // handle not destroyed in destructor -> detached
            // handle destroyed automatically after execution (final_suspend returns suspend_never)
        }

        DetachedTask(const DetachedTask&) = delete;
        auto operator=(const DetachedTask&) -> DetachedTask& = delete;
        DetachedTask(DetachedTask&& other) noexcept = delete;
        auto operator=(DetachedTask&& other) noexcept -> DetachedTask& = delete;

        inline auto getHandle() const -> const handle_type& { return m_handle; }

      private:
        handle_type m_handle;
    };

    template<typename T>
    class Task
    {
      public:
        using promise_type = detail::TaskPromise<T>;
        using handle_type = std::coroutine_handle<promise_type>;

        Task(handle_type handle)
          : m_handle{ handle }
        {
        }
        ~Task()
        {
            if (m_handle) {
                m_handle.destroy();
            }
        }

        Task(const Task&) = delete;
        auto operator=(const Task&) -> Task& = delete;

        Task(Task&& other) noexcept
          : m_handle{ std::exchange(other.m_handle, nullptr) }
        {
        }
        auto operator=(Task&& other) noexcept -> Task&
        {
            if (this != &other) {
                if (m_handle) {
                    m_handle.destroy();
                }
                m_handle = std::exchange(other.m_handle, nullptr);
            }
            return *this;
        }

        auto await_ready() const noexcept -> bool
        {
            // check if result is available

            return !m_handle || m_handle.done();
        }
        auto await_suspend(std::coroutine_handle<> waiter) noexcept -> std::coroutine_handle<>
        {
            // called when paused (waiter = paused coroutine)

            m_handle.promise().waiter = waiter;
            return m_handle;
        }
        auto await_resume() -> T
        {
            // called after the task completes to produce the result of co_await

            if (m_handle.promise().exception) {
                std::rethrow_exception(m_handle.promise().exception);
            }

            if constexpr (!std::is_void_v<T>) {
                if (!m_handle.promise().value) {
                    throw std::runtime_error("Broken Promise: Task completed without returning a value.");
                }
                return std::move(*m_handle.promise().value);
            }
        }

        auto getHandle() const -> const handle_type& { return m_handle; }

      private:
        handle_type m_handle;
    };

    namespace detail
    {
        struct DetachedTaskPromise
        {
            auto get_return_object() -> DetachedTask
            {
                // create the coroutine (DetachedTask)

                return { DetachedTask::handle_type::from_promise(*this) };
            }
            auto initial_suspend() -> std::suspend_always
            {
                // coroutine execution not started -> caller has to manually resume

                return std::suspend_always{};
            }
            auto final_suspend() noexcept -> std::suspend_never
            {
                // destroy coroutine when its finished (fire-and-forget)

                return std::suspend_never{};
            }
            auto unhandled_exception() -> void
            {
                // terminate program on unhandled exception

                std::terminate();
            }
            auto return_void() -> void
            {
                // co_return with no value
            }
        };

        template<typename T>
        struct TaskPromiseBase
        {
            std::coroutine_handle<> waiter{};
            std::exception_ptr exception{};
            IExecutor* executor{ nullptr };

            template<typename... Args>
            TaskPromiseBase(IExecutor& ex, Args&&...)
              : executor(&ex)
            {
            }
            template<typename Class, typename... Args>
            TaskPromiseBase(Class&&, IExecutor& ex, Args&&...)
              : executor(&ex)
            {
            }
            TaskPromiseBase() = default;

            auto get_return_object() -> Task<T>
            {
                // create the coroutine (Task)

                return Task<T>::handle_type::from_promise(static_cast<TaskPromise<T>&>(*this));
            }
            auto initial_suspend() -> std::suspend_always
            {
                // coroutine execution not started -> caller has to manually resume

                return std::suspend_always{};
            }
            auto final_suspend() noexcept
            {
                // coroutine has finished executing its body
                // suspend here (returning false in await_ready) to prevent the coroutine frame from being
                // destroyed immediately -> allow the consumer (waiter) to retrieve the result (value or
                // exception) stored in this promise
                // await_suspend performs a symmetric control transfer to the waiter coroutine if one is
                // registered

                struct
                {
                    auto await_ready() noexcept -> bool { return false; }
                    auto await_suspend(Task<T>::handle_type handle) noexcept -> std::coroutine_handle<>
                    {
                        return handle.promise().waiter ? handle.promise().waiter : std::noop_coroutine();
                    }
                    auto await_resume() noexcept -> void {}
                } awaiter{};
                return awaiter;
            }
            auto unhandled_exception() -> void
            {
                // store unhandled exception for later rethrow

                exception = std::current_exception();
            }
            template<typename U>
            auto await_transform(Task<U>&& childTask) -> Task<U>&&
            {
                // handle co_await another task inside the coroutine
                // -> propagate execution context to the child

                if (childTask.getHandle()) {
                    if (executor) {
                        childTask.getHandle().promise().executor = executor;
                    }
                }
                return std::move(childTask);
            }
            template<typename U>
            auto await_transform(U&& task) -> U&&
            {
                return std::forward<U>(task);
            }
        };

        template<typename T = void>
        struct TaskPromise : public TaskPromiseBase<T>
        {
            using TaskPromiseBase<T>::TaskPromiseBase;

            std::optional<T> value{};
            auto return_value(T val) -> void
            {
                // store the value provided by co_return

                value.emplace(std::move(val));
            }
        };

        template<>
        struct TaskPromise<void> : public TaskPromiseBase<void>
        {
            using TaskPromiseBase<void>::TaskPromiseBase;

            auto return_void() -> void
            {
                // co_return with no value
            }
        };
    }
}