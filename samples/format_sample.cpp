#include "reflect.hpp"

#include <concepts>
#include <format>
#include <memory>
#include <print>

namespace detail
{

    template<size_t N>
    struct FixedString
    {
        char buff[N]{};
        constexpr operator std::string_view() const { return { buff, N - 1 }; }
    };

    template<typename T>
    consteval auto class_format_size() -> size_t
    {
        auto size{ 0uz };
        size += 2;                              // "[ "
        size += reflect::type_name<T>().size(); // "TestStruct"
        size += 5;                              // ": {{ "

        reflect::for_each<T>([&size](const auto I) {
            size += reflect::member_name<I, T>().size(); // "i"
            size += 4;                                   // ": {}"
            if constexpr (I < reflect::size<T>() - 1) {
                size += 2; // ", "
            }
        });

        size += 5; // " }} ]"
        size += 1; // "\0"
        return size;
    }

    template<typename T>
    consteval auto class_format()
    {
        constexpr auto size{ class_format_size<T>() };
        FixedString<size> out;
        auto* iter = out.buff;

        auto append = [&](std::string_view s) {
            for (char c : s)
                *iter++ = c;
        };

        append("[ ");
        append(reflect::type_name<T>());
        append(": {{ ");

        reflect::for_each<T>([&](const auto I) {
            append(reflect::member_name<I, T>());
            append(": {}");
            if constexpr (I < reflect::size<T>() - 1) {
                append(", ");
            }
        });

        append(" }} ]");
        append("\0");
        return out;
    }

    // clang-format off
    template <typename T> struct is_std_utility : std::false_type {};
    template <typename... Args> struct is_std_utility<std::tuple<Args...>> : std::true_type {};
    template <typename T, typename U> struct is_std_utility<std::pair<T, U>> : std::true_type {};
    template <typename T> struct is_std_utility<std::allocator<T>> : std::true_type {};

    template <typename T> struct is_smart_ptr : std::false_type {};
    template <typename T> struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {};
    template <typename T> struct is_smart_ptr<std::shared_ptr<T>> : std::true_type {};

    template <typename T>
    concept Reflectable = 
        std::is_class_v<T> &&
        requires { reflect::size<T>(); } &&
        !std::ranges::range<T> &&
        !is_std_utility<std::remove_cvref_t<T>>::value &&
        !is_smart_ptr<std::remove_cvref_t<T>>::value;
    // clang-format on
}

template<typename T>
    requires std::is_class_v<T>
struct std::formatter<T>
{
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const T& t, auto& ctx) const
    {
        auto make_arg = []<typename Field>(Field&& field) -> decltype(auto) {
            using Decayed = std::remove_cvref_t<Field>;
            if constexpr (std::is_same_v<Decayed, const char*> || std::is_same_v<Decayed, char*>) {
                return std::forward<Field>(field);
            }
            else if constexpr (std::is_pointer_v<Decayed>) {
                return static_cast<const void*>(field);
            }
            else {
                return std::forward<Field>(field);
            }
        };

        return [&]<size_t... Is>(std::index_sequence<Is...>) {
            auto args{ std::forward_as_tuple(make_arg(reflect::get<Is>(t))...) };
            return std::apply([&](const auto&... args) {
                return std::vformat_to(ctx.out(), detail::class_format<T>(), std::make_format_args(args...));
            }, args);
        }(std::make_index_sequence<reflect::size<T>()>{});
    }
};

struct TestStruct
{
    int i;
    float f;
};

struct TestStruct2
{
    TestStruct* tsp;
    TestStruct ts;
    double d;
    short s;
};

int main()
{
    // std::println("{}", reflect::size<TestStruct>());
    // std::println("{}", format_size<TestStruct>());
    // std::println("{}", std::string(format<TestStruct>()));

    TestStruct ts{};
    std::println("{}", ts);
    std::println("{}", TestStruct2{ &ts });
    return 0;
}
