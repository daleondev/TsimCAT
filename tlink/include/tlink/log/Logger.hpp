#pragma once

#include "tlink/coroutine/coroutine.hpp"
#include "tlink/log/format.hpp"

#include <chrono>
#include <format>
#include <print>
#include <ranges>
#include <source_location>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>

namespace tlink::log
{
    struct FunctionInfo
    {
        std::string_view storage_specifier{};  // static
        std::string_view function_specifier{}; // virtual
        std::string_view constexpr_specifier{};
        std::string_view return_type{};

        std::string_view full_name{};
        std::string_view short_name{};
        std::string_view parameter_list{};

        std::string_view const_qualifier{};
        std::string_view ref_qualifier{}; // & or &&

        std::string_view template_arguments{};
    };

    namespace detail
    {
        constexpr uint64_t hash(std::string_view str)
        {
            uint64_t hash = 0xcbf29ce484222325; // FNV offset basis
            for (char c : str) {
                hash ^= static_cast<unsigned char>(c);
                hash *= 0x100000001b3; // FNV prime
            }
            return hash;
        }

        consteval uint64_t operator""_h(const char* str, size_t len)
        {
            return hash(std::string_view(str, len));
        }

        consteval auto parseFunctionName(std::string_view function_name) -> FunctionInfo
        {
            FunctionInfo info{};

            if (function_name.empty() || function_name.find('(') == std::string_view::npos) {
                return info;
            }

            auto isLambda{ function_name.contains("lambda") };

            auto parts{ std::views::split(function_name, ' ') };

            auto find_part_with = [&parts](const char c) {
                return std::ranges::find_if(
                  parts, [&](auto&& part) { return std::ranges::find(part, c) != part.end(); });
            };

            // full function name
            auto name_part{ find_part_with('(') };
            auto name_token{ std::string_view((*name_part).begin(), (*name_part).end()) };
            auto name_end{ name_token.find('(') };
            info.full_name = name_token.substr(0, name_end);

            // short function name
            info.short_name = info.full_name;
            if (info.full_name.contains(':')) {
                auto name_start{ info.full_name.rfind(':') };
                info.short_name = info.full_name.substr(name_start + 1);
            }

            // parameter list
            auto parameter_part{ name_part };
            if (!name_token.contains("()")) {
                if (name_token.contains(')')) {
                    info.parameter_list = name_token.substr(name_end + 1, name_token.size() - name_end - 2);
                }
                else {
                    auto first_parameter{ name_token.substr(name_end + 1, name_token.size() - name_end - 1) };

                    parameter_part = find_part_with(')');
                    auto parameter_token{ std::string_view((*parameter_part).begin(),
                                                           (*parameter_part).end()) };
                    auto parameter_end{ parameter_token.find(')') };

                    auto start_ptr{ first_parameter.data() };
                    auto end_ptr{ parameter_token.data() + parameter_end };
                    info.parameter_list = std::string_view(start_ptr, end_ptr - start_ptr);
                }
            }

            // potential storage/function/constexpr specifier
            auto current_part{ parts.begin() };
            auto storage_function_constexpr_specifier_token{ std::string_view((*current_part).begin(),
                                                                              (*current_part).end()) };
            switch (hash(storage_function_constexpr_specifier_token)) {
                case "static"_h:
                    info.storage_specifier = storage_function_constexpr_specifier_token;
                    std::ranges::advance(current_part, 1, parts.end());
                    break;
                case "virtual"_h:
                    info.function_specifier = storage_function_constexpr_specifier_token;
                    std::ranges::advance(current_part, 1, parts.end());
                    break;
                case "constexpr"_h:
                    info.constexpr_specifier = storage_function_constexpr_specifier_token;
                    std::ranges::advance(current_part, 1, parts.end());
                    break;
            }

            // potential constexpr specifier if storage specifier exists
            if (!info.storage_specifier.empty() && info.constexpr_specifier.empty()) {
                auto constexpr_specifier_token{ std::string_view((*current_part).begin(),
                                                                 (*current_part).end()) };
                if (constexpr_specifier_token == "constexpr") {
                    info.constexpr_specifier = constexpr_specifier_token;
                    std::ranges::advance(current_part, 1, parts.end());
                }
            }

            // return type
            auto return_type_token{ std::string_view((*current_part).begin(), (*current_part).end()) };
            auto start_ptr{ return_type_token.data() };
            auto end_ptr{ name_token.data() };
            info.return_type =
              end_ptr > start_ptr ? std::string_view(start_ptr, end_ptr - start_ptr - 1) : "";

            // const qualifier
            current_part = std::ranges::next(parameter_part, 1, parts.end());
            if (current_part != parts.end()) {
                auto const_qualifier_token{ std::string_view((*current_part).begin(),
                                                             (*current_part).end()) };
                if (const_qualifier_token == "const") {
                    info.const_qualifier = const_qualifier_token;
                    std::ranges::advance(current_part, 1, parts.end());
                }
            }

            // ref qualifier
            if (current_part != parts.end()) {
                auto ref_qualifier_token{ std::string_view((*current_part).begin(), (*current_part).end()) };
                if (ref_qualifier_token.contains("&")) {
                    info.ref_qualifier = ref_qualifier_token;
                }
            }

            // template parameters
            if (function_name.contains("with")) {
                using namespace std::literals;
                auto template_start{ function_name.find_last_of('[') + ("[with "sv).size() };
                info.template_arguments =
                  function_name.substr(template_start, function_name.size() - template_start - 1);
            }

            return info;
        }

        static_assert(parseFunctionName("void test()").full_name == "test");
        static_assert(parseFunctionName("void test()").return_type == "void");
        static_assert(parseFunctionName("void MyClass::test() const &").const_qualifier == "const");
        static_assert(parseFunctionName("void MyClass::test() const &").ref_qualifier == "&");
        static_assert(parseFunctionName("void MyClass::test() &&").ref_qualifier == "&&");
        static_assert(parseFunctionName("static void test()").storage_specifier == "static");
        static_assert(parseFunctionName("static constexpr void test()").constexpr_specifier == "constexpr");
        static_assert(parseFunctionName("MyClass()").return_type == "");
        static_assert(parseFunctionName("MyClass()").full_name == "MyClass");
    }

    enum class Level
    {
        Debug,
        Info,
        Warning,
        Error
    };

    struct LogEntry
    {
        std::chrono::system_clock::time_point timestamp;
        Level level;
        std::string message;
        std::thread::id threadId;
        std::string file;
        uint32_t line;
        FunctionInfo function;
    };

    struct LoggerConfig
    {
        bool showTimestamp{ true };
        std::string timestampFormat{ "{:%Y-%m-%d %H:%M:%S}" };
        bool showLevel{ true };
        bool showThreadId{ true };
        bool showFile{ false };
        bool showLine{ false };
        bool showFunction{ false };
    };

    template<typename... Args>
    struct FormatString
    {
        std::format_string<Args...> str;
        std::source_location loc;
        FunctionInfo function;

        template<typename T>
            requires std::convertible_to<const T&, std::string_view>
        consteval FormatString(const T& s, std::source_location l = std::source_location::current())
          : str(s)
          , loc(l)
          , function(detail::parseFunctionName(l.function_name()))
        {
        }
    };

    class Logger
    {
      public:
        static auto instance() -> Logger&
        {
            static Logger logger;
            return logger;
        }

        Logger()
          : m_thread([this]() { m_ctx.run(); })
        {
            coro::co_spawn(m_ctx, [this](coro::IExecutor&) -> coro::Task<void> { return process(); });
        }

        ~Logger()
        {
            m_channel.close();
            m_ctx.stop();
            if (m_thread.joinable()) {
                m_thread.join();
            }
        }

        auto setConfig(LoggerConfig config) -> void { m_config = std::move(config); }

        template<typename... Args>
        auto log(Level level,
                 std::source_location loc,
                 const FunctionInfo& function,
                 std::format_string<Args...> fmt,
                 Args&&... args) -> void
        {
            try {
                auto msg = std::format(fmt, std::forward<Args>(args)...);
                m_channel.push({ std::chrono::system_clock::now(),
                                 level,
                                 std::move(msg),
                                 std::this_thread::get_id(),
                                 loc.file_name(),
                                 loc.line(),
                                 function });
            } catch (...) {
            }
        }

      private:
        auto process() -> coro::Task<void>
        {
            while (true) {
                auto entry = co_await m_channel.next();
                if (!entry) {
                    break;
                }
                print(*entry);
            }
        }

        auto print(const LogEntry& entry) -> void
        {
            if (m_config.showTimestamp) {
                try {
                    auto ts = std::chrono::floor<std::chrono::milliseconds>(entry.timestamp);
                    std::print("[{}] ", std::vformat(m_config.timestampFormat, std::make_format_args(ts)));
                } catch (...) {
                    std::print("[Timestamp Error] ");
                }
            }

            if (m_config.showLevel) {
                std::print("[{}] ", entry.level);
            }

            if (m_config.showThreadId) {
                std::stringstream ss;
                ss << entry.threadId;
                std::print("[Thread {}] ", ss.str());
            }

            if (m_config.showFile || m_config.showLine || m_config.showFunction) {
                std::print("[");
                bool first = true;
                if (m_config.showFile) {
                    std::print("{}", entry.file);
                    first = false;
                }
                if (m_config.showLine) {
                    if (!first)
                        std::print(":");
                    std::print("{}", entry.line);
                    first = false;
                }
                if (m_config.showFunction) {
                    if (!first)
                        std::print(" ");
                    std::print("in {}", entry.function);
                }
                std::print("] ");
            }

            std::println("{}", entry.message);
        }

        LoggerConfig m_config{};
        coro::Channel<LogEntry> m_channel{};
        coro::Context m_ctx{};
        std::thread m_thread;
    };

    template<typename... Args>
    auto debug(FormatString<std::type_identity_t<Args>...> fmt, Args&&... args) -> void
    {
        Logger::instance().log(Level::Debug, fmt.loc, fmt.function, fmt.str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto info(FormatString<std::type_identity_t<Args>...> fmt, Args&&... args) -> void
    {
        Logger::instance().log(Level::Info, fmt.loc, fmt.function, fmt.str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto warning(FormatString<std::type_identity_t<Args>...> fmt, Args&&... args) -> void
    {
        Logger::instance().log(Level::Warning, fmt.loc, fmt.function, fmt.str, std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto error(FormatString<std::type_identity_t<Args>...> fmt, Args&&... args) -> void
    {
        Logger::instance().log(Level::Error, fmt.loc, fmt.function, fmt.str, std::forward<Args>(args)...);
    }
}
