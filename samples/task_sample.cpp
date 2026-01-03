#include <atomic>
#include <concepts>
#include <condition_variable>
#include <coroutine>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <iostream>
#include <print>
#include <thread>
#include <type_traits>
#include <utility>

class IExecutor
{
  public:
    virtual ~IExecutor() = default;
    virtual void schedule(std::coroutine_handle<> handle) = 0;
    virtual void run() = 0;
    virtual void stop() = 0;
};

template<typename T>
concept Executor = std::is_base_of_v<IExecutor, T>;

class DetachedTask
{
  public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type
    {
        DetachedTask get_return_object() { return DetachedTask{ handle_type::from_promise(*this) }; }
        std::suspend_always initial_suspend() { return {}; }
        auto final_suspend() noexcept
        {
            struct HandleDestroyer
            {
                bool await_ready() noexcept { return false; }
                void await_suspend(std::coroutine_handle<> handle) noexcept { handle.destroy(); }
                void await_resume() noexcept {}
            };
            return HandleDestroyer{};
        }
        void unhandled_exception() { std::terminate(); }
        void return_void() {}
    };

    explicit DetachedTask(handle_type handle)
      : m_handle(handle)
    {
    }
    inline const handle_type& getHandle() const { return m_handle; }

  private:
    handle_type m_handle;
};

template<typename T>
struct TaskPromise;

template<typename T>
class Task
{
  public:
    using promise_type = TaskPromise<T>;
    using handle_type = std::coroutine_handle<promise_type>;

    Task(handle_type h)
      : m_handle(h)
    {
    }
    ~Task() { m_handle.destroy(); }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept
      : m_handle(other.m_handle)
    {
        other.m_handle = nullptr;
    }
    Task& operator=(Task&& other) noexcept
    {
        if (this != &other) {
            if (m_handle)
                m_handle.destroy();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    bool await_ready() const noexcept { return !m_handle || m_handle.done(); }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> waiter) noexcept
    {
        m_handle.promise().waiter = waiter;
        return m_handle;
    }
    T await_resume()
    {
        if constexpr (!std::is_void_v<T>) {
            return std::move(m_handle.promise().value);
        }
        else {
            return;
        }
    }

    inline const handle_type& getHandle() const { return m_handle; }

  private:
    handle_type m_handle;

    template<typename U>
    friend class TaskPromiseBase;
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

    Task<T> get_return_object()
    {
        return Task<T>::handle_type::from_promise(static_cast<TaskPromise<T>&>(*this));
    }
    std::suspend_always initial_suspend() { return {}; }
    auto final_suspend() noexcept
    {
        struct
        {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(Task<T>::handle_type handle) noexcept
            {
                return handle.promise().waiter ? handle.promise().waiter : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        } awaiter{};
        return awaiter;
    }
    void unhandled_exception() { exception = std::current_exception(); }
    template<typename U>
    Task<U>&& await_transform(Task<U>&& childTask)
    {
        auto childHandle = childTask.m_handle;
        if (childHandle) {
            if (executor) {
                childHandle.promise().executor = this->executor;
            }
        }
        return std::move(childTask);
    }
    template<typename U>
    U&& await_transform(U&& awaitable)
    {
        return std::forward<U>(awaitable);
    }
};

template<typename T = void>
struct TaskPromise : public TaskPromiseBase<T>
{
    using TaskPromiseBase<T>::TaskPromiseBase;

    T value;
    void return_value(T val) { value = std::move(val); }
};

template<>
struct TaskPromise<void> : public TaskPromiseBase<void>
{
    using TaskPromiseBase<void>::TaskPromiseBase;

    void return_void() {}
};

class CoroContext : public IExecutor
{
  public:
    CoroContext() = default;
    ~CoroContext() = default;
    CoroContext(const CoroContext&) = delete;
    CoroContext& operator=(const CoroContext&) = delete;
    CoroContext(CoroContext&&) = delete;
    CoroContext& operator=(CoroContext&&) = delete;

    void run() override
    {
        while (m_running) {
            std::unique_lock lock(m_mutex);

            while (!m_queue.empty()) {
                auto handle = std::move(m_queue.front());
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

    void stop() override
    {
        m_running = false;
        m_cv.notify_all();
    }

    void schedule(std::coroutine_handle<> handle) override
    {
        {
            std::lock_guard lock(m_mutex);
            m_queue.push_back(handle);
        }
        m_cv.notify_one();
    }

  private:
    std::atomic<bool> m_running{ true };
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<std::coroutine_handle<>> m_queue;
};

template<typename T>
class Channel
{
    struct NextAwaiter;

  public:
    void push(T value)
    {
        std::unique_lock lock(m_mutex);
        m_queue.push_back(std::move(value));

        if (m_coro.handle) {
            auto [handle, exec] = std::exchange(m_coro, { nullptr, nullptr });
            lock.unlock();
            if (exec) {
                exec->schedule(handle);
            }
            else {
                handle.resume();
            }
        };
    }

    auto next() -> NextAwaiter { return { *this }; }

  private:
    std::mutex m_mutex;
    std::deque<T> m_queue;
    struct
    {
        std::coroutine_handle<> handle;
        IExecutor* executor;
    } m_coro{ nullptr, nullptr };

    struct NextAwaiter
    {
        Channel& channel;

        bool await_ready()
        {
            std::lock_guard lock(channel.m_mutex);
            return !channel.m_queue.empty();
        }

        template<typename P>
        bool await_suspend(std::coroutine_handle<P> handle)
        {
            std::lock_guard lock(channel.m_mutex);

            if (!channel.m_queue.empty()) {
                return false;
            }

            channel.m_coro.handle = handle;
            if constexpr (requires { handle.promise().executor; }) {
                channel.m_coro.executor = handle.promise().executor;
            }
            else {
                channel.m_coro.executor = nullptr;
            }

            return true;
        }

        T await_resume()
        {
            std::lock_guard lock(channel.m_mutex);
            T val = std::move(channel.m_queue.front());
            channel.m_queue.pop_front();
            return val;
        }
    };
};

static Channel<int> g_channel;

template<typename Ex, typename Coro>
DetachedTask co_spawn_impl(Ex& ex, Coro coro)
{
    co_await std::invoke(std::move(coro), ex);
}

template<Executor Ex, std::invocable<Ex&> Coro>
void co_spawn(Ex& ex, Coro&& coro)
{
    auto detached = co_spawn_impl(ex, std::forward<Coro>(coro));
    ex.schedule(detached.getHandle());
}

Task<int> sub_coro()
{
    std::println("Sub Start on: {}", std::this_thread::get_id());
    int val = co_await g_channel.next();
    std::println("Sub Resumed on: {} with value {}", std::this_thread::get_id(), val);
    co_return val;
}

Task<void> coro(IExecutor& ex)
{
    std::println("Start on: {}", std::this_thread::get_id());
    for (int i = 0; i < 3; ++i) {
        int val = co_await sub_coro();
        std::println("Resumed on: {} with value {}", std::this_thread::get_id(), val);
    }
    ex.stop();
}

int main()
{
    CoroContext ctx;
    co_spawn(ctx, coro);

    std::thread producer([] {
        for (int i = 0; i < 3; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::print("Pushing from: {} -> ", std::this_thread::get_id());
            g_channel.push(i);
        }
    });

    ctx.run();

    if (producer.joinable())
        producer.join();

    return 0;
}