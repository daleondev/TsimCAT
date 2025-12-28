#pragma once
#include <coroutine>
#include <exception>
#include <utility>

namespace tlink {

/**
 * @brief A basic coroutine return type for TLink asynchronous tasks.
 * This is a "lazy" task: it doesn't start until it's co_awaited.
 */
template<typename T = void>
struct Task {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        T result;
        std::coroutine_handle<> waiter;

        Task get_return_object() { return Task{handle_type::from_promise(*this)}; }
        std::suspend_always initial_suspend() { return {}; }
        
        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(handle_type h) noexcept {
                return h.promise().waiter ? h.promise().waiter : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        FinalAwaiter final_suspend() noexcept { return {}; }

        void return_value(T value) { result = std::move(value); }
        void unhandled_exception() { std::terminate(); }
    };

    handle_type handle;

    explicit Task(handle_type h) : handle(h) {}
    ~Task() { if (handle) handle.destroy(); }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept : handle(std::exchange(other.handle, nullptr)) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = std::exchange(other.handle, nullptr);
        }
        return *this;
    }

    // Awaiter interface
    bool await_ready() const noexcept { return !handle || handle.done(); }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> waiter) noexcept {
        handle.promise().waiter = waiter;
        return handle;
    }

    T await_resume() { return std::move(handle.promise().result); }
};

/**
 * @brief Specialization of Task for void return type.
 */
template<>
struct Task<void> {
    struct promise_type {
        std::coroutine_handle<> waiter;

        Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() { return {}; }
        
        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                return h.promise().waiter ? h.promise().waiter : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        FinalAwaiter final_suspend() noexcept { return {}; }

        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> handle;

    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~Task() { if (handle) handle.destroy(); }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept : handle(std::exchange(other.handle, nullptr)) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = std::exchange(other.handle, nullptr);
        }
        return *this;
    }

    bool await_ready() const noexcept { return !handle || handle.done(); }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> waiter) noexcept {
        handle.promise().waiter = waiter;
        return handle;
    }

    void await_resume() {}
};

} // namespace tlink
