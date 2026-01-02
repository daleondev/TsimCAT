#pragma once

#include <reflect>

#include <concepts>
#include <format>
#include <memory>

namespace tlink::log::detail
{
    template<typename T>
    consteval auto is_std_type() -> bool;

    template<typename T>
    consteval auto class_format();

    template<typename T, size_t Level = 0>
    consteval auto class_pretty_format();

    template<typename T>
    concept Reflectable = std::is_class_v<T> && !is_std_type<T>();

    template<typename T>
    concept ScopedEnum = std::is_scoped_enum_v<T>;

    template<typename T>
    concept SmartPtr = requires(T p) {
        typename T::element_type;
        { p.get() } -> std::convertible_to<const void*>;
        requires !std::is_aggregate_v<T>;
    };

    template<size_t N>
    struct FixedString
    {
        char buff[N]{};
        constexpr operator std::string_view() const { return { buff, N - 1 }; }
    };
}

template<tlink::log::detail::Reflectable T>
struct std::formatter<T>
{
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
        auto make_arg = []<typename Field>(Field&& field) -> decltype(auto) {
            using Type = std::remove_cvref_t<Field>;

            if constexpr (std::is_same_v<Type, const char*> || std::is_same_v<Type, char*>) {
                return std::forward<Field>(field);
            }
            else if constexpr (tlink::log::detail::SmartPtr<Type>) {
                return static_cast<const void*>(field.get());
            }
            else if constexpr (std::is_pointer_v<Type>) {
                return static_cast<const void*>(field);
            }
            else if constexpr (!std::formattable<Type, char>) {
                static constexpr std::string_view missing = "-";
                return missing;
            }
            else {
                return std::forward<Field>(field);
            }
        };

        auto make_tuple_args = []<typename... Args>(Args&&... args) {
            using Tuple = std::tuple<std::conditional_t<std::is_lvalue_reference_v<Args>, Args,
                                                        std::remove_reference_t<Args>>...>;
            return Tuple(std::forward<Args>(args)...);
        };

        if (pretty) {
            auto make_flat_tuple = [make_arg_fn = std::move(make_arg),
                                    make_tuple_args_fn = std::move(make_tuple_args)]<typename Field>(
                                     this auto&& make_flat_tuple_recursive, Field&& field) {
                using Type = std::remove_cvref_t<Field>;

                if constexpr (tlink::log::detail::Reflectable<Type>) {
                    return [&]<size_t... Is>(std::index_sequence<Is...>) {
                        return std::tuple_cat(make_flat_tuple_recursive(reflect::get<Is>(field))...);
                    }(std::make_index_sequence<reflect::size<Type>()>{});
                }
                else {
                    return make_tuple_args_fn(make_arg_fn(std::forward<Field>(field)));
                }
            };

            auto args = [&]<size_t... Is>(std::index_sequence<Is...>) {
                return std::tuple_cat(make_flat_tuple(reflect::get<Is>(t))...);
            }(std::make_index_sequence<reflect::size<T>()>{});

            return std::apply([&ctx](const auto&... args) {
                return std::vformat_to(
                  ctx.out(), tlink::log::detail::class_pretty_format<T>(), std::make_format_args(args...));
            }, args);
        }
        else {
            auto args = [&]<size_t... Is>(std::index_sequence<Is...>) {
                return make_tuple_args(make_arg(reflect::get<Is>(t))...);
            }(std::make_index_sequence<reflect::size<T>()>{});

            return std::apply([&ctx](const auto&... args) {
                return std::vformat_to(ctx.out(),
                                       tlink::log::detail::class_format<T>(),
                                       std::make_format_args(args...));
            }, args);
        }
    }
};

template<tlink::log::detail::ScopedEnum T>
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

template<tlink::log::detail::SmartPtr T>
struct std::formatter<T> : std::formatter<const void*>
{
    template<typename Ctx>
    auto format(const T& t, Ctx& ctx) const -> Ctx::iterator
    {
        return std::formatter<const void*>::format(t.get(), ctx);
    }
};

namespace tlink::log::detail
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

    static constexpr std::string_view INDENT = "  ";

    template<typename T>
    consteval auto class_format_size() -> size_t
    {
        auto size{ 0uz };
        size += 2;                              // "[ "
        size += reflect::type_name<T>().size(); // "<class>"
        size += 5;                              // ": {{ "

        reflect::for_each<T>([&size](const auto I) {
            size += reflect::member_name<I, T>().size(); // "<member>"
            size += 4;                                   // ": {}"
            if constexpr (I < reflect::size<T>() - 1) {
                size += 2; // ", "
            }
        });

        size += 6; // " }} ]\0"
        return size;
    }

    template<typename T>
    consteval auto class_format()
    {
        FixedString<class_format_size<T>()> out;
        auto* iter{ out.buff };

        auto append = [&](std::string_view s) {
            for (char c : s) {
                *iter++ = c;
            }
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

        append(" }} ]\0");
        return out;
    }

    template<typename T, size_t Level = 0>
    consteval auto class_pretty_format_size() -> size_t
    {
        auto size{ 0uz };
        size += 3; // "{{\n"

        reflect::for_each<T>([&size](const auto I) {
            using MemberType = decltype(typename reflect::member_type<I, T>());

            size += (Level + 1) * INDENT.size();
            size += reflect::member_name<I, T>().size(); // "<member>"
            if constexpr (Reflectable<MemberType>) {
                size += 2; // ": "
                size += class_pretty_format_size<MemberType, Level + 1>();
            }
            else {
                size += 4; // ": {}"
            }
            if constexpr (I < reflect::size<T>() - 1) {
                size += 1; // ","
            }
            size += 1; // "\n"
        });

        size += Level * INDENT.size();
        size += 3; // "}}\0"
        return size;
    }

    template<typename T, size_t Level>
    consteval auto class_pretty_format()
    {
        FixedString<class_pretty_format_size<T, Level>()> out;
        auto* iter{ out.buff };

        auto append = [&](std::string_view s) {
            for (char c : s) {
                *iter++ = c;
            }
        };

        append("{{\n");

        reflect::for_each<T>([&](const auto I) {
            using MemberType = decltype(typename reflect::member_type<I, T>());

            for (auto i{ 0uz }; i < (Level + 1); ++i) {
                append(INDENT);
            }
            append(reflect::member_name<I, T>());

            if constexpr (Reflectable<MemberType>) {
                append(": ");
                append(class_pretty_format<MemberType, Level + 1>());
            }
            else {
                append(": {}");
            }

            if constexpr (I < reflect::size<T>() - 1) {
                append(",");
            }
            append("\n");
        });

        for (auto i{ 0uz }; i < Level; ++i) {
            append(INDENT);
        }
        append("}}\0");
        return out;
    }
}