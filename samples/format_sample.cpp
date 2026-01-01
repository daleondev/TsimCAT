#include "reflect.hpp"

#include <chrono>
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
    template<typename T>
    consteval auto namespace_name() noexcept -> std::string_view
    {
        using type_name_info = reflect::detail::type_name_info<std::remove_pointer_t<std::remove_cvref_t<T>>>;
        constexpr std::string_view function_name{
            reflect::detail::function_name<std::remove_pointer_t<std::remove_cvref_t<T>>>()
        };
        constexpr std::string_view qualified_type_name{ 
            function_name.substr(type_name_info::begin, function_name.find(type_name_info::end) - type_name_info::begin) 
        };
        constexpr std::string_view tmp_type_name{ 
            qualified_type_name.substr(0, qualified_type_name.find_first_of("<", 1)) 
        };
        constexpr std::string_view namespace_name{ 
            tmp_type_name.substr(0, tmp_type_name.find_last_of("::") + 1) 
        };
        return namespace_name;
    }
    // clang-format on

    template<typename T>
    consteval auto is_std_type() -> bool
    {
        return namespace_name<T>().starts_with("std::");
    }

    template<typename T>
    concept Reflectable = std::is_class_v<T> && !is_std_type<T>();

    template<typename T>
    concept ScopedEnum = std::is_scoped_enum_v<T>;
}

template<detail::Reflectable T>
struct std::formatter<T>
{
    bool pretty{ false };

    constexpr auto parse(auto& ctx) -> std::remove_reference_t<decltype(ctx)>::iterator
    {
        auto it{ ctx.begin() };
        if (it == ctx.end()) {
            return it;
        }

        if (*it == 'p') {
            pretty = true;
            ++it;
        }

        if (it != ctx.end() && *it != '}') {
            throw std::format_error("Invalid format args");
        }

        return it;
    }

    auto format(const T& t, auto& ctx) const -> std::remove_reference_t<decltype(ctx)>::iterator
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
            return std::apply([&]<typename... Args>(Args&&... args) {
                return std::vformat_to(
                  ctx.out(), detail::class_format<T>(), std::make_format_args(std::forward<Args>(args)...));
            }, args);
        }(std::make_index_sequence<reflect::size<T>()>{});
    }
};

template<detail::ScopedEnum T>
struct std::formatter<T>
{
    bool verbose{ false };

    constexpr auto parse(auto& ctx) -> std::remove_reference_t<decltype(ctx)>::iterator
    {
        auto it{ ctx.begin() };
        if (it == ctx.end()) {
            return it;
        }

        if (*it == 'v') {
            verbose = true;
            ++it;
        }

        if (it != ctx.end() && *it != '}') {
            throw std::format_error("Invalid format args");
        }

        return it;
    }

    auto format(T t, auto& ctx) const -> std::remove_reference_t<decltype(ctx)>::iterator
    {
        if (verbose) {
            return std::format_to(ctx.out(), "{}:{}", reflect::type_name(t), reflect::enum_name(t));
        }
        return std::format_to(ctx.out(), "{}", reflect::enum_name(t));
    }
};

int main()
{
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

    enum class MyEnum
    {
        A,
        B,
        C
    };

    TestStruct ts{};
    std::println("{}", ts);
    std::println("{}", TestStruct2{ &ts });
    std::println("{}", MyEnum::B);
    std::println("{:v}", MyEnum::C);

    std::array<int, 3> a = { 1, 2, 3 };
    std::println("{}", a);

    std::pair p = { 3, 4uz };
    std::println("{}", p);
    // std::println(
    //   "{}",
    //   reflect::detail::function_name<std::remove_pointer_t<std::remove_cvref_t<std::pair<int, int>>>>());

    return 0;
}
