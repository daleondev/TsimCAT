#include <print>
#include <iostream>
#include <coroutine>
#include <exception>
#include <type_traits>
#include <cstdint>
#include <semaphore>

template <typename T>
struct TaskPromise;

template <typename T>
class Task
{
public:
    using promise_type = TaskPromise<T>;
    using handle_type = std::coroutine_handle<promise_type>;

    Task(handle_type h) : m_handle(h) {}
    ~Task()
    {
        m_handle.destroy();
    }

    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;

    Task(Task &&other) noexcept : m_handle(other.m_handle)
    {
        other.m_handle = nullptr;
    }

    Task &operator=(Task &&other) noexcept
    {
        if (this != &other)
        {
            if (m_handle)
                m_handle.destroy();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    auto await_ready() const noexcept -> bool
    {
        return !m_handle || m_handle.done();
    }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> waiter) noexcept
    {
        m_handle.promise().waiter = waiter;
        return m_handle;
    }
    T await_resume()
    {
        if constexpr (!std::is_void_v<T>)
        {
            return std::move(m_handle.promise().value);
        }
    }

    inline const handle_type &getHandle() const { return m_handle; }

private:
    handle_type m_handle;
};

template <typename T>
struct TaskPromiseBase
{
    std::coroutine_handle<> waiter;
    std::exception_ptr exception;

    Task<T> get_return_object()
    {
        return Task<T>::handle_type::from_promise(
            static_cast<TaskPromise<T> &>(*this));
    }
    std::suspend_always initial_suspend()
    {
        return {};
    }
    auto final_suspend() noexcept
    {
        struct Awaiter
        {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(Task<T>::handle_type h) noexcept
            {
                return h.promise().waiter ? h.promise().waiter : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        return Awaiter{};
    }
    void unhandled_exception()
    {
        exception = std::current_exception();
    }
};

template <typename T = void>
struct TaskPromise : public TaskPromiseBase<T>
{
    T value;

    void return_value(T val)
    {
        value = std::move(val);
    }
};

template <>
struct TaskPromise<void> : public TaskPromiseBase<void>
{
    void return_void() {}
};

Task<int> test()
{
    co_return 5;
}

Task<void> run()
{
    int i = co_await test();
    std::cout << "Result: " << i << std::endl;
    co_return;
}

struct SyncWaitTask
{
    struct promise_type
    {
        std::binary_semaphore sem{0};

        SyncWaitTask get_return_object() { return {this}; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { std::terminate(); }
        void return_void()
        {
            sem.release();
        }
    };
    promise_type *promise;
};

void sync_wait(Task<void> &&task)
{
    auto wrapper = [&](Task<void> t) -> SyncWaitTask
    {
        co_await t;
    }(std::move(task));
    wrapper.promise->get_return_object();
    std::coroutine_handle<SyncWaitTask::promise_type>::from_promise(*wrapper.promise).resume();
    wrapper.promise->sem.acquire();
}

int main()
{
    sync_wait(run());
    return 0;
}