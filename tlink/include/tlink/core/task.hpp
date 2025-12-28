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

        auto get_return_object() -> Task { return Task{handle_type::from_promise(*this)}; }
        auto initial_suspend() -> std::suspend_always { return {}; }
        
        struct FinalAwaiter {
            auto await_ready() noexcept -> bool { return false; }
            auto await_suspend(handle_type h) noexcept -> std::coroutine_handle<> {
                return h.promise().waiter ? h.promise().waiter : std::noop_coroutine();
            }
            auto await_resume() noexcept -> void {}
        };
        auto final_suspend() noexcept -> FinalAwaiter { return {}; }

        auto return_value(T value) -> void { result = std::move(value); }
        auto unhandled_exception() -> void { std::terminate(); }
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
    auto await_ready() const noexcept -> bool { return !handle || handle.done(); }

    auto await_suspend(std::coroutine_handle<> waiter) noexcept -> std::coroutine_handle<> {
        handle.promise().waiter = waiter;
        return handle;
    }

    auto await_resume() -> T { return std::move(handle.promise().result); }
};

/**
 * @brief Specialization of Task for void return type.
 */
template<>
struct Task<void> {
    struct promise_type {
        std::coroutine_handle<> waiter;

        auto get_return_object() -> Task { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        auto initial_suspend() -> std::suspend_always { return {}; }
        
        struct FinalAwaiter {
            auto await_ready() noexcept -> bool { return false; }
            auto await_suspend(std::coroutine_handle<promise_type> h) noexcept -> std::coroutine_handle<> {
                return h.promise().waiter ? h.promise().waiter : std::noop_coroutine();
            }
            auto await_resume() noexcept -> void {}
        };
        auto final_suspend() noexcept -> FinalAwaiter { return {}; }

        auto return_void() -> void {}
        auto unhandled_exception() -> void { std::terminate(); }
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

    auto await_ready() const noexcept -> bool { return !handle || handle.done(); }

    auto await_suspend(std::coroutine_handle<> waiter) noexcept -> std::coroutine_handle<> {
        handle.promise().waiter = waiter;
        return handle;
    }

    auto await_resume() -> void {}
};

} // namespace tlink
