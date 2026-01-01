#include <coroutine>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <print>

template<typename T>
struct GeneratorPromise;

template<typename T>
class Generator
{
  public:
    using promise_type = GeneratorPromise<T>;
    using handle_type = std::coroutine_handle<promise_type>;

    Generator(handle_type h)
      : m_handle(h)
    {
    }
    ~Generator() { m_handle.destroy(); }

    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;

    Generator(Generator&& other) noexcept
      : m_handle(other.m_handle)
    {
        other.m_handle = nullptr;
    }

    Generator& operator=(Generator&& other) noexcept
    {
        if (this != &other) {
            if (m_handle)
                m_handle.destroy();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    explicit operator bool()
    {
        next();
        return !m_handle.done();
    }
    T operator()()
    {
        auto& value{ [this]() -> T& {
            next();
            return m_handle.promise().value;
        }() };

        if (m_handle.done()) {
            throw std::runtime_error("Generator exhausted");
        }

        m_requestNext = true;
        return std::move(value);
    }

  private:
    void next()
    {
        if (m_requestNext && !m_handle.done()) {
            m_handle.resume();
            if (m_handle.promise().exception) {
                std::rethrow_exception(m_handle.promise().exception);
            }
            m_requestNext = false;
        }
    }

    bool m_requestNext{ true };
    handle_type m_handle;
};

template<typename T>
struct GeneratorPromise
{
    T value;
    std::exception_ptr exception;

    Generator<T> get_return_object() { return Generator<T>::handle_type::from_promise(*this); }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { exception = std::current_exception(); }

    template<std::convertible_to<T> From>
    std::suspend_always yield_value(From&& from)
    {
        value = std::forward<From>(from);
        return {};
    }

    void return_void() {}
};

Generator<std::uint64_t> fibonacci_sequence(unsigned n)
{
    if (n == 0)
        co_return;

    if (n > 94)
        throw std::runtime_error("Too big Fibonacci sequence. Elements would overflow.");

    co_yield 0;

    if (n == 1)
        co_return;

    co_yield 1;

    if (n == 2)
        co_return;

    std::uint64_t a = 0;
    std::uint64_t b = 1;

    for (unsigned i = 2; i < n; ++i) {
        std::uint64_t s = a + b;
        co_yield s;
        a = b;
        b = s;
    }
}

int main()
{
    try {
        auto gen = fibonacci_sequence(10); // max 94 before uint64_t overflows

        for (int j = 0; gen; ++j)
            std::println("fib({})={}", j, gen());
    } catch (const std::exception& ex) {
        std::println(std::cerr, "Exception: {}", ex.what());
    } catch (...) {
        std::println(std::cerr, "Unknown exception");
    }
    return 0;
}