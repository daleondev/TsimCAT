#include "Logger.hpp"

#include <chrono>
#include <print>
#include <thread>

namespace core::logger
{
    auto Logger::instance() -> Logger&
    {
        static Logger s_instance;
        return s_instance;
    }

    Logger::~Logger()
    {
        if (m_fileStream.is_open()) {
            m_fileStream.close();
        }
    }

    auto Logger::init(std::filesystem::path logFile) -> result::Result<void>
    {
        std::scoped_lock lock(m_mutex);
        if (!logFile.empty()) {
            if (logFile.has_parent_path()) {
                std::filesystem::create_directories(logFile.parent_path());
            }
            m_fileStream.open(logFile, std::ios::app);
            if (!m_fileStream) {
                return std::unexpected(std::make_error_code(std::errc::io_error));
            }
            m_fileEnabled = true;
        }
        return result::success();
    }

    auto Logger::logImpl(Level level, const std::source_location& loc, std::string_view msg) -> void
    {
        // Use system time directly (UTC) to avoid dependency on tzdata/ICU which can cause crashes on MinGW
        // if missing. For local time, one would typically use std::chrono::current_zone(), but we fallback to
        // simple reliable formatting.
        auto now = std::chrono::system_clock::now();
        auto timeStr = std::format("{:%T}", std::chrono::floor<std::chrono::seconds>(now));

        std::string_view levelStr;
        std::string_view colorCode;
        switch (level) {
            case Level::Debug:
                levelStr = "DEBUG";
                colorCode = "\033[37m";
                break;
            case Level::Info:
                levelStr = "INFO";
                colorCode = "\033[32m";
                break;
            case Level::Warning:
                levelStr = "WARNING";
                colorCode = "\033[33m";
                break;
            case Level::Error:
                levelStr = "ERRROR";
                colorCode = "\033[31m";
                break;
        }

        std::scoped_lock lock(m_mutex);

        // Console
        std::println(std::cout,
                     "{} {} [{}:{}] {}",
                     timeStr,
                     std::format("{}{}\033[0m", colorCode, levelStr),
                     std::filesystem::path(loc.file_name()).filename().string(),
                     loc.line(),
                     msg);

        // File
        if (m_fileEnabled) {
            std::println(m_fileStream,
                         "{} {} [{}:{}] {}",
                         timeStr,
                         levelStr,
                         std::filesystem::path(loc.file_name()).filename().string(),
                         loc.line(),
                         msg);
            m_fileStream.flush();
        }
    }
}
