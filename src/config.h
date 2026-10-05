#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "logger/logger_api.h"

namespace logger::detail
{

enum class ERollPolicy
{
    kNone,
    kSize,
    kDaily,
    kHourly,
    kSizeAndDaily,
};

struct SinkConfig
{
    std::vector<std::string> sinks{"console"};
    std::string file_path;
    bool file_append{true};
    std::uint64_t file_max_size{0};
    std::size_t file_max_backups{5};
    ERollPolicy file_roll_policy{ERollPolicy::kNone};
    bool file_roll_policy_configured{false};
    std::size_t file_max_age_days{0};
};

enum class ELogFormat
{
    kText,
    kJson,
};

struct FormatterConfig
{
    ELogFormat format{ELogFormat::kText};
    std::string pattern;
    bool pattern_configured{false};
};

struct LoggerConfig
{
    ELogLevel root_level{ELogLevel::kDebug};
    std::unordered_map<std::string, ELogLevel> module_levels;
    SinkConfig sink_config;
    FormatterConfig formatter_config;
};

bool parseConfigFile(const std::string& path, LoggerConfig& config, std::string& error);

}  // namespace logger::detail
