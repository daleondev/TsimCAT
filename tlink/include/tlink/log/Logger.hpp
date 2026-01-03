#pragma once

#include "tlink/coroutine/Channel.hpp"
#include "tlink/coroutine/Task.hpp"
#include "tlink/log/format.hpp"

#include <chrono>
#include <format>
#include <print>
#include <sstream>
#include <string>
#include <thread>

namespace tlink::log
{
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
    };

    struct LoggerConfig
    {
        bool showTimestamp{ true };
        std::string timestampFormat{ "{:%Y-%m-%d %H:%M:%S}" };
        bool showLevel{ true };
        bool showThreadId{ true };
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

        ~Logger()
        {
            m_channel.close();
        }

        auto setConfig(LoggerConfig config) -> void
        {
            // Simple thread-safety for config replacement (atomic swap would be better but this is okay for
            // now)
            m_config = std::move(config);
        }

        template<typename... Args>
        auto log(Level level, std::format_string<Args...> fmt, Args&&... args) -> void
        {
            try {
                auto msg = std::format(fmt, std::forward<Args>(args)...);
                m_channel.push(
                  { std::chrono::system_clock::now(), level, std::move(msg), std::this_thread::get_id() });
            }
            catch (...) {
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
                }
                catch (...) {
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

            std::println("{}", entry.message);
        }

        coro::Channel<LogEntry> m_channel;
        coro::DetachedTask m_worker;
        LoggerConfig m_config;
    };

    template<typename... Args>
    auto debug(std::format_string<Args...> fmt, Args&&... args) -> void
    {
        Logger::instance().log(Level::Debug, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto info(std::format_string<Args...> fmt, Args&&... args) -> void
    {
        Logger::instance().log(Level::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto warning(std::format_string<Args...> fmt, Args&&... args) -> void
    {
        Logger::instance().log(Level::Warning, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    auto error(std::format_string<Args...> fmt, Args&&... args) -> void
    {
        Logger::instance().log(Level::Error, fmt, std::forward<Args>(args)...);
    }
}
