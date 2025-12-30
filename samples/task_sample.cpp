#include <print>
#include <iostream>
#include <coroutine>
#include <exception>
#include <type_traits>
#include <cstdint>
#include <condition_variable>
#include <atomic>
#include <deque>
#include <concepts>
#include <functional>

class DetachedTask
{
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type
    {
        DetachedTask get_return_object() { return DetachedTask{handle_type::from_promise(*this)}; }
        std::suspend_always initial_suspend() { return {}; }
        auto final_suspend() noexcept
        {
            struct HandleDestroyer
            {
                bool await_ready() noexcept { return false; }
                void await_suspend(std::coroutine_handle<> handle) noexcept
                {
                    handle.destroy();
                }
                void await_resume() noexcept {}
            };
            return HandleDestroyer{};
        }
        void unhandled_exception() { std::terminate(); }
        void return_void() {}
    };

    explicit DetachedTask(handle_type handle) : m_handle(handle) {}
    inline const handle_type &getHandle() const { return m_handle; }

private:
    handle_type m_handle;
};

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

    bool await_ready() const noexcept
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
            std::coroutine_handle<> await_suspend(Task<T>::handle_type handle) noexcept
            {
                return handle.promise().waiter ? handle.promise().waiter : std::noop_coroutine();
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

class CoroContext
{
public:
    CoroContext() = default;
    ~CoroContext() = default;
    CoroContext(const CoroContext &) = delete;
    CoroContext &operator=(const CoroContext &) = delete;
    CoroContext(CoroContext &&) = delete;
    CoroContext &operator=(CoroContext &&) = delete;

    void run()
    {
        while (m_running)
        {
            std::unique_lock lock(m_mutex);

            while (!m_queue.empty())
            {
                auto handle = std::move(m_queue.front());
                m_queue.pop_front();

                lock.unlock();
                if (handle && !handle.done())
                {
                    handle.resume();
                }
                lock.lock();
            }

            m_cv.wait(lock, [this]
                      { return !m_queue.empty() || !m_running; });
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

private:
    std::atomic<bool> m_running{true};
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<std::coroutine_handle<>> m_queue;
};

template <typename T>
concept Executor = requires(T t, std::coroutine_handle<> h) {
    { t.run() };
    { t.stop() };
    { t.schedule(h) };
};

template <Executor Exe, std::invocable<Exe &> Coro>
void co_spawn(Exe &exe, Coro &&coro)
{
    auto detached = [&exe, c = std::forward<Coro>(coro)]() mutable -> DetachedTask
    {
        co_await std::invoke(std::move(c), exe);
    }();
    exe.schedule(detached.getHandle());
}

Task<int> coro()
{
    co_return 5;
}

int main()
{
    CoroContext ctx;
    co_spawn(ctx, [](Executor auto &exe) -> Task<void>
             {
        auto i = co_await coro();
        std::println("{}", i);
        exe.stop(); });
    ctx.run();
    return 0;
}