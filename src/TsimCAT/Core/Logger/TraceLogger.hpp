#pragma once

#include "Logger.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace core::logger
{
    enum class TraceCategory
    {
        Lifecycle,
        Protocol,
        State,
        Flow,
        Invariant
    };

    struct TraceField
    {
        std::string key;
        std::string value;
    };

    struct TraceConfig
    {
        bool enabled{ false };
        bool enableProtocol{ true };
        bool enableState{ true };
        bool enableFlow{ true };
        bool enableInvariant{ true };
        bool mirrorToHumanLog{ true };
        int sampleIntervalMs{ 0 };
        std::filesystem::path outputFile{ "analysis/session/protocol_trace.jsonl" };
        std::vector<std::string> stationFilter;
    };

    class TraceLogger
    {
      public:
        static auto instance() -> TraceLogger&;

        auto init(TraceConfig config) -> result::Result<void>;
        auto shutdown() -> void;

        auto event(TraceCategory category,
                   std::string_view station,
                   std::string_view event,
                   std::initializer_list<TraceField> fields = {}) -> void;

#ifndef emit
        auto emit(TraceCategory category,
                  std::string_view station,
                  std::string_view event,
                  std::initializer_list<TraceField> fields = {}) -> void
        {
            this->event(category, station, event, fields);
        }
#endif

        auto enabledFor(TraceCategory category, std::string_view station) const -> bool;

      private:
        TraceLogger() = default;

        auto categoryEnabled(TraceCategory category) const -> bool;
        auto categoryName(TraceCategory category) const -> std::string_view;
        auto shouldSample(std::string_view station, std::string_view event) -> bool;

        TraceConfig m_config{};
        std::mutex m_mutex;
        std::ofstream m_stream;
        std::unordered_set<std::string> m_stationFilter;
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_lastEmit;
    };

    auto traceField(std::string key, std::string value) -> TraceField;
    auto traceField(std::string key, std::string_view value) -> TraceField;
    auto traceField(std::string key, const char* value) -> TraceField;
    auto traceField(std::string key, bool value) -> TraceField;
    auto traceField(std::string key, int value) -> TraceField;
    auto traceField(std::string key, uint32_t value) -> TraceField;
    auto traceField(std::string key, uint64_t value) -> TraceField;
    auto traceField(std::string key, double value) -> TraceField;
}
