#pragma once

#include "Common/Result.hpp"
#include "format_utils.hpp"

#include <filesystem>
#include <fstream>
#include <mutex>
#include <source_location>
#include <string_view>
#include <format>

namespace core::logger
{
    enum class Level
    {
        Debug,
        Info,
        Warning,
        Error
    };

    struct LogMsg
    {
        std::string_view fmt;
        std::source_location loc;

        consteval LogMsg(const char* s, std::source_location l = std::source_location::current())
          : fmt(s)
          , loc(l)
        {
        }
        consteval LogMsg(std::string_view s, std::source_location l = std::source_location::current())
          : fmt(s)
          , loc(l)
        {
        }
    };

    class Logger
    {
      public:
        static auto instance() -> Logger&;

        auto init(std::filesystem::path logFile = "") -> result::Result<void>;

        template<typename... Args>
        auto log(Level level,
                 std::source_location loc,
                 std::string_view fmt,
                 Args&&... args) -> void
        {
            auto msg = std::vformat(fmt, std::make_format_args(args...));
            logImpl(level, loc, msg);
        }

      private:
        Logger() = default;
        ~Logger();

        auto logImpl(Level level, const std::source_location& loc, std::string_view msg) -> void;

        std::mutex m_mutex;
        std::ofstream m_fileStream;
        bool m_fileEnabled{ false };
    };
    
    template<typename... Args>
    void debug(LogMsg msg, Args&&... args)
    {
        Logger::instance().log(Level::Debug, msg.loc, msg.fmt, args...);
    }

    template<typename... Args>
    void info(LogMsg msg, Args&&... args)
    {
        Logger::instance().log(Level::Info, msg.loc, msg.fmt, args...);
    }

    template<typename... Args>
    void warn(LogMsg msg, Args&&... args)
    {
        Logger::instance().log(Level::Warning, msg.loc, msg.fmt, args...);
    }

    template<typename... Args>
    void error(LogMsg msg, Args&&... args)
    {
        Logger::instance().log(Level::Error, msg.loc, msg.fmt, args...);
    }
}