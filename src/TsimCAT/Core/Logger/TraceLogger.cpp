#include "TraceLogger.hpp"

#include <chrono>
#include <format>

namespace core::logger
{
    namespace
    {
        auto toJsonEscaped(std::string_view value) -> std::string
        {
            std::string escaped;
            escaped.reserve(value.size() + 8);
            for (const auto ch : value) {
                switch (ch) {
                    case '\\':
                        escaped += "\\\\";
                        break;
                    case '"':
                        escaped += "\\\"";
                        break;
                    case '\n':
                        escaped += "\\n";
                        break;
                    case '\r':
                        escaped += "\\r";
                        break;
                    case '\t':
                        escaped += "\\t";
                        break;
                    default:
                        escaped += ch;
                        break;
                }
            }
            return escaped;
        }
    }

    auto TraceLogger::instance() -> TraceLogger&
    {
        static TraceLogger instance;
        return instance;
    }

    auto TraceLogger::init(TraceConfig config) -> result::Result<void>
    {
        std::scoped_lock lock(m_mutex);
        m_config = std::move(config);
        m_stationFilter.clear();
        m_lastEmit.clear();

        if (!m_config.enabled) {
            if (m_stream.is_open()) {
                m_stream.close();
            }
            return result::success();
        }

        for (const auto& station : m_config.stationFilter) {
            if (!station.empty()) {
                m_stationFilter.insert(station);
            }
        }

        if (m_config.outputFile.has_parent_path()) {
            std::filesystem::create_directories(m_config.outputFile.parent_path());
        }

        m_stream.open(m_config.outputFile, std::ios::trunc);
        if (!m_stream) {
            return std::unexpected(std::make_error_code(std::errc::io_error));
        }

        info("TraceLogger initialized at {}", m_config.outputFile.string());
        return result::success();
    }

    auto TraceLogger::shutdown() -> void
    {
        std::scoped_lock lock(m_mutex);
        if (m_stream.is_open()) {
            m_stream.flush();
            m_stream.close();
        }
    }

    auto TraceLogger::categoryEnabled(TraceCategory category) const -> bool
    {
        switch (category) {
            case TraceCategory::Lifecycle:
                return true;
            case TraceCategory::Protocol:
                return m_config.enableProtocol;
            case TraceCategory::State:
                return m_config.enableState;
            case TraceCategory::Flow:
                return m_config.enableFlow;
            case TraceCategory::Invariant:
                return m_config.enableInvariant;
            default:
                return false;
        }
    }

    auto TraceLogger::enabledFor(TraceCategory category, std::string_view station) const -> bool
    {
        if (!m_config.enabled || !categoryEnabled(category)) {
            return false;
        }

        if (!m_stationFilter.empty() && !m_stationFilter.contains(std::string(station))) {
            return false;
        }
        return true;
    }

    auto TraceLogger::categoryName(TraceCategory category) const -> std::string_view
    {
        switch (category) {
            case TraceCategory::Lifecycle:
                return "lifecycle";
            case TraceCategory::Protocol:
                return "protocol";
            case TraceCategory::State:
                return "state";
            case TraceCategory::Flow:
                return "flow";
            case TraceCategory::Invariant:
                return "invariant";
            default:
                return "unknown";
        }
    }

    auto TraceLogger::shouldSample(std::string_view station, std::string_view event) -> bool
    {
        if (m_config.sampleIntervalMs <= 0) {
            return true;
        }

        const auto key = std::format("{}::{}", station, event);
        const auto now = std::chrono::steady_clock::now();
        const auto it = m_lastEmit.find(key);

        if (it == m_lastEmit.end()) {
            m_lastEmit.emplace(key, now);
            return true;
        }

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
        if (elapsed.count() >= m_config.sampleIntervalMs) {
            it->second = now;
            return true;
        }

        return false;
    }

    auto TraceLogger::event(TraceCategory category,
                            std::string_view station,
                            std::string_view event,
                            std::initializer_list<TraceField> fields) -> void
    {
        std::scoped_lock lock(m_mutex);

        if (!enabledFor(category, station) || !m_stream.is_open() || !shouldSample(station, event)) {
            return;
        }

        const auto now = std::chrono::system_clock::now();
        const auto timestamp = std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::milliseconds>(now));

        std::string payload = "{";
        payload += std::format("\"ts\":\"{}\",", toJsonEscaped(timestamp));
        payload += std::format("\"category\":\"{}\",", categoryName(category));
        payload += std::format("\"station\":\"{}\",", toJsonEscaped(station));
        payload += std::format("\"event\":\"{}\"", toJsonEscaped(event));

        if (!fields.size()) {
            payload += "}";
        }
        else {
            payload += ",\"fields\":{";
            bool first = true;
            for (const auto& field : fields) {
                if (!first) {
                    payload += ",";
                }
                first = false;
                payload += std::format("\"{}\":\"{}\"", toJsonEscaped(field.key), toJsonEscaped(field.value));
            }
            payload += "}}";
        }

        m_stream << payload << '\n';
        m_stream.flush();

        if (m_config.mirrorToHumanLog) {
            debug("TRACE {} {} {}", station, categoryName(category), event);
        }
    }

    auto traceField(std::string key, std::string value) -> TraceField
    {
        return TraceField{ .key = std::move(key), .value = std::move(value) };
    }

    auto traceField(std::string key, std::string_view value) -> TraceField
    {
        return TraceField{ .key = std::move(key), .value = std::string(value) };
    }

    auto traceField(std::string key, const char* value) -> TraceField
    {
        return TraceField{ .key = std::move(key), .value = value ? std::string(value) : std::string() };
    }

    auto traceField(std::string key, bool value) -> TraceField
    {
        return TraceField{ .key = std::move(key), .value = value ? "true" : "false" };
    }

    auto traceField(std::string key, int value) -> TraceField
    {
        return TraceField{ .key = std::move(key), .value = std::to_string(value) };
    }

    auto traceField(std::string key, uint32_t value) -> TraceField
    {
        return TraceField{ .key = std::move(key), .value = std::to_string(value) };
    }

    auto traceField(std::string key, uint64_t value) -> TraceField
    {
        return TraceField{ .key = std::move(key), .value = std::to_string(value) };
    }

    auto traceField(std::string key, double value) -> TraceField
    {
        return TraceField{ .key = std::move(key), .value = std::format("{:.6f}", value) };
    }
}
