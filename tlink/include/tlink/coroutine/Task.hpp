#pragma once

#include "Context.hpp"

#include <cstdlib>
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
          : m_handle(handle)
        {
        }
        ~DetachedTask()
        {
            // handle not destroyed in destructor -> detached
            // handle later destroyed in promise_type::final_suspend()
        }

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

        Task(handle_type h)
          : m_handle(h)
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
          : m_handle(other.m_handle)
        {
            other.m_handle = nullptr;
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

        auto await_ready() const noexcept -> bool { return !m_handle || m_handle.done(); }
        auto await_suspend(std::coroutine_handle<> waiter) noexcept -> std::coroutine_handle<>
        {
            m_handle.promise().waiter = waiter;
            return m_handle;
        }
        auto await_resume() -> T
        {
            if (m_handle.promise().exception) {
                std::rethrow_exception(m_handle.promise().exception);
            }

            if constexpr (!std::is_void_v<T>) {
                if (!m_handle.promise().value) {
                     throw std::runtime_error("Broken Promise: Task completed without returning a value.");
                }
                return std::move(*m_handle.promise().value);
            }
            else {
                return;
            }
        }

        inline auto getHandle() -> handle_type& { return m_handle; }
        inline auto getHandle() const -> const handle_type& { return m_handle; }

      private:
        handle_type m_handle;
    };

    namespace detail
    {
        struct DetachedTaskPromise
        {
            auto get_return_object() -> DetachedTask
            {
                return DetachedTask{ DetachedTask::handle_type::from_promise(*this) };
            }
            auto initial_suspend() -> std::suspend_always { return {}; }
            auto final_suspend() noexcept -> std::suspend_never { return {}; }
            auto unhandled_exception() -> void { std::abort(); }
            auto return_void() -> void {}
        };

        template<typename T>
        struct TaskPromiseBase
        {
            std::coroutine_handle<> waiter;
            std::exception_ptr exception;
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
                return Task<T>::handle_type::from_promise(static_cast<TaskPromise<T>&>(*this));
            }
            auto initial_suspend() -> std::suspend_always { return {}; }
            auto final_suspend() noexcept
            {
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
            auto unhandled_exception() -> void { exception = std::current_exception(); }
            template<typename U>
            auto await_transform(Task<U>&& childTask) -> Task<U>&&
            {
                // propagate execution context to child
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

            std::optional<T> value;
            auto return_value(T val) -> void { value.emplace(std::move(val)); }
        };

        template<>
        struct TaskPromise<void> : public TaskPromiseBase<void>
        {
            using TaskPromiseBase<void>::TaskPromiseBase;

            auto return_void() -> void {}
        };
    }
}