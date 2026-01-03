#pragma once

#include <reflect>

#include <array>
#include <concepts>
#include <format>

namespace tlink::log::format
{
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
    concept Reflectable =
      std::is_class_v<std::remove_cvref_t<T>> && std::is_aggregate_v<std::remove_cvref_t<T>> &&
      !is_std_type<std::remove_cvref_t<T>>();

    template<typename T>
    concept ScopedEnum = std::is_scoped_enum_v<std::remove_cvref_t<T>>;

    template<typename T>
    concept VoidPtr = std::is_pointer_v<std::remove_cvref_t<T>> &&
                      std::is_void_v<std::remove_pointer_t<std::remove_cvref_t<T>>>;

    template<typename T>
    concept CharPtr =
      std::is_pointer_v<std::remove_cvref_t<T>> &&
      std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<T>>>, char>;

    template<typename T>
    concept ValuePtr = std::is_pointer_v<std::remove_cvref_t<T>> && !CharPtr<T> && !VoidPtr<T> &&
                       std::formattable<typename std::remove_pointer_t<std::remove_cvref_t<T>>, char>;

    template<typename T>
    concept SmartPtr = requires(std::remove_cvref_t<T> p) {
        typename std::remove_cvref_t<T>::element_type;
        { p.get() } -> std::convertible_to<const void*>;
        requires !std::is_aggregate_v<std::remove_cvref_t<T>>;
        requires std::formattable<typename std::remove_cvref_t<T>::element_type, char>;
    };

    template<typename T>
    consteval auto class_format_size() -> size_t
    {
        using Type = std::remove_cvref_t<T>;
        auto size{ 0uz };
        size += 2;                                 // "[ "
        size += reflect::type_name<Type>().size(); // "<class>"
        size += 5;                                 // ": {{ "

        reflect::for_each<Type>([&size](const auto I) {
            size += reflect::member_name<I, Type>().size(); // "<member>"
            size += 4;                                      // ": {}"
            if constexpr (I < reflect::size<Type>() - 1) {
                size += 2; // ", "
            }
        });

        size += 5; // " }} ]"
        return size;
    }

    template<typename T>
    consteval auto class_format()
    {
        using Type = std::remove_cvref_t<T>;
        std::array<char, class_format_size<Type>()> fmt{};

        auto iter{ fmt.begin() };
        auto append = [&](std::string_view s) {
            for (char c : s) {
                *iter++ = c;
            }
        };

        append("[ ");
        append(reflect::type_name<Type>());
        append(": {{ ");

        reflect::for_each<Type>([&](const auto I) {
            append(reflect::member_name<I, Type>());
            append(": {}");
            if constexpr (I < reflect::size<Type>() - 1) {
                append(", ");
            }
        });

        append(" }} ]");
        return reflect::fixed_string<char, fmt.size()>(fmt.data());
    }

    static constexpr std::string_view PRETTY_INDENT{ "  " };

    template<typename T, size_t Level = 0>
    consteval auto class_pretty_format_size() -> size_t
    {
        using Type = std::remove_cvref_t<T>;
        auto size{ 0uz };
        size += 3; // "{{\n"

        reflect::for_each<Type>([&size](const auto I) {
            using MemberType = decltype(typename reflect::member_type<I, Type>());

            size += (Level + 1) * PRETTY_INDENT.size();
            size += reflect::member_name<I, Type>().size(); // "<member>"
            if constexpr (Reflectable<MemberType>) {
                size += 2; // ": "
                size += class_pretty_format_size<MemberType, Level + 1>();
            }
            else {
                size += 4; // ": {}"
            }
            if constexpr (I < reflect::size<Type>() - 1) {
                size += 1; // ","
            }
            size += 1; // "\n"
        });

        size += Level * PRETTY_INDENT.size();
        size += 2; // "}}"
        return size;
    }

    template<typename T, size_t Level = 0>
    consteval auto class_pretty_format()
    {
        using Type = std::remove_cvref_t<T>;
        std::array<char, class_pretty_format_size<Type, Level>()> fmt{};

        auto iter{ fmt.begin() };
        auto append = [&](std::string_view s) {
            for (char c : s) {
                *iter++ = c;
            }
        };

        append("{{\n");

        reflect::for_each<Type>([&](const auto I) {
            using MemberType = decltype(typename reflect::member_type<I, Type>());

            for (auto i{ 0uz }; i < (Level + 1); ++i) {
                append(PRETTY_INDENT);
            }
            append(reflect::member_name<I, Type>());

            if constexpr (Reflectable<MemberType>) {
                append(": ");
                append(class_pretty_format<MemberType, Level + 1>());
            }
            else {
                append(": {}");
            }

            if constexpr (I < reflect::size<Type>() - 1) {
                append(",");
            }
            append("\n");
        });

        for (auto i{ 0uz }; i < Level; ++i) {
            append(PRETTY_INDENT);
        }
        append("}}");
        return reflect::fixed_string<char, fmt.size()>(fmt.data());
    }
}

template<tlink::log::format::Reflectable T>
struct std::formatter<T>
{
    static_assert(
      requires { T{}; },
      "Type T contains reference members or other non-value-initializable members, which are not supported "
      "for automatic formatting. Consider removing references or providing default initializers.");

    bool pretty{ false };

    template<typename Ctx>
    constexpr auto parse(Ctx& ctx) -> Ctx::iterator
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

    template<typename Ctx>
    auto format(const T& t, Ctx& ctx) const -> Ctx::iterator
    {
        auto check_arg = []<typename Field>(Field&& field) -> decltype(auto) {
            if constexpr (!std::formattable<Field, char>) {
                return "-";
            }
            else {
                return std::forward<Field>(field);
            }
        };

        auto make_tuple_args = []<typename... Args>(Args&&... args) {
            using Tuple = std::tuple<
              std::conditional_t<std::is_lvalue_reference_v<Args>, Args, std::remove_reference_t<Args>>...>;
            return Tuple(std::forward<Args>(args)...);
        };

        if (pretty) {
            auto make_flat_tuple_args = [check_arg = std::move(check_arg),
                                         make_tuple_args = std::move(make_tuple_args)]<typename Field>(
                                          this auto&& make_flat_tuple_args, Field&& field) {
                if constexpr (tlink::log::format::Reflectable<Field>) {
                    using Type = std::remove_cvref_t<Field>;
                    return [&]<size_t... Is>(std::index_sequence<Is...>) {
                        return std::tuple_cat(make_flat_tuple_args(reflect::get<Is>(field))...);
                    }(std::make_index_sequence<reflect::size<Type>()>{});
                }
                else {
                    return make_tuple_args(check_arg(std::forward<Field>(field)));
                }
            };

            auto args = [&]<size_t... Is>(std::index_sequence<Is...>) {
                return std::tuple_cat(make_flat_tuple_args(reflect::get<Is>(t))...);
            }(std::make_index_sequence<reflect::size<T>()>{});

            static constexpr auto fmt{ tlink::log::format::class_pretty_format<T>() };
            return std::apply([&ctx](const auto&... args) { return std::format_to(ctx.out(), fmt, args...); },
                              args);
        }
        else {
            auto args = [&]<size_t... Is>(std::index_sequence<Is...>) {
                return make_tuple_args(check_arg(reflect::get<Is>(t))...);
            }(std::make_index_sequence<reflect::size<T>()>{});

            static constexpr auto fmt{ tlink::log::format::class_format<T>() };
            return std::apply([&ctx](const auto&... args) { return std::format_to(ctx.out(), fmt, args...); },
                              args);
        }
    }
};

template<tlink::log::format::ScopedEnum T>
struct std::formatter<T>
{
    bool verbose{ false };

    template<typename Ctx>
    constexpr auto parse(Ctx& ctx) -> Ctx::iterator
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

    template<typename Ctx>
    auto format(T t, Ctx& ctx) const -> Ctx::iterator
    {
        if (verbose) {
            return std::format_to(ctx.out(), "{}:{}", reflect::type_name(t), reflect::enum_name(t));
        }
        return std::format_to(ctx.out(), "{}", reflect::enum_name(t));
    }
};

template<std::formattable<char> T>
struct std::formatter<std::optional<T>> : std::formatter<T>
{
    using Base = std::formatter<T>;

    template<typename Ctx>
    auto format(const std::optional<T>& t, Ctx& ctx) const -> Ctx::iterator
    {
        if (!t) {
            return std::ranges::copy("[ null ]"sv, ctx.out()).out;
        }

        ctx.advance_to(std::ranges::copy("[ "sv, ctx.out()).out);
        ctx.advance_to(Base::format(*t, ctx));
        return std::ranges::copy(" ]"sv, ctx.out()).out;
    }
};

template<tlink::log::format::ValuePtr T>
struct std::formatter<T> : std::formatter<std::remove_pointer_t<std::remove_cvref_t<T>>>
{
    using Base = std::formatter<std::remove_pointer_t<std::remove_cvref_t<T>>>;

    template<typename Ctx>
    auto format(T t, Ctx& ctx) const -> Ctx::iterator
    {
        if (!t) {
            return std::format_to(ctx.out(), "[ ({}) -> {} ]", static_cast<const void*>(t), "null");
        }

        ctx.advance_to(std::format_to(ctx.out(), "[ ({}) -> ", static_cast<const void*>(t)));
        ctx.advance_to(Base::format(*t, ctx));
        return std::ranges::copy(" ]"sv, ctx.out()).out;
    }
};

template<tlink::log::format::SmartPtr T>
struct std::formatter<T> : std::formatter<typename T::element_type*>
{
    template<typename Ctx>
    auto format(const T& t, Ctx& ctx) const -> Ctx::iterator
    {
        return std::formatter<typename T::element_type*>::format(t.get(), ctx);
    }
};
