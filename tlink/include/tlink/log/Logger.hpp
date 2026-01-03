#pragma once

#include "tlink/coroutine/Channel.hpp"
#include "tlink/coroutine/Task.hpp"
#include "tlink/log/format.hpp"

#include <chrono>
#include <format>
#include <print>
#include <source_location>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>

namespace tlink::log
{
    namespace detail
    {
        consteval auto parseFunctionName(std::string_view func) -> std::string_view
        {
            auto is_alnum = [](char c) {
                return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
            };

            if (func.empty())
                return "";

            auto parenPos = func.find('(');
            if (parenPos == std::string_view::npos)
                return func;

            auto end = parenPos;
            while (end > 0 && func[end - 1] == ' ')
                --end;

            auto start = end;
            while (true) {
                while (start > 0 && is_alnum(func[start - 1]))
                    --start;

                if (start >= 2 && func[start - 1] == ':' && func[start - 2] == ':') {
                    start -= 2;
                }
                else {
                    break;
                }
            }

            return func.substr(start, end - start);
        }
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
        std::string function;
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
        std::string_view function;

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
          : m_worker{ process() }
        {
            m_worker.getHandle().resume();
        }

        ~Logger() { m_channel.close(); }

        auto setConfig(LoggerConfig config) -> void { m_config = std::move(config); }

        template<typename... Args>
        auto log(Level level,
                 std::source_location loc,
                 std::string_view function,
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
                                 std::string(function) });
            } catch (...) {
            }
        }

      private:
        auto process() -> coro::DetachedTask
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

        coro::Channel<LogEntry> m_channel;
        coro::DetachedTask m_worker;
        LoggerConfig m_config;
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
