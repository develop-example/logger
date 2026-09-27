#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "logger/logger_api.h"

namespace logger::detail
{

struct SinkConfig
{
    std::vector<std::string> sinks{"console"};
    std::string file_path;
    bool file_append{true};
};

struct LoggerConfig
{
    ELogLevel root_level{ELogLevel::kDebug};
    std::unordered_map<std::string, ELogLevel> module_levels;
    SinkConfig sink_config;
};

bool parseConfigFile(const std::string& path, LoggerConfig& config, std::string& error);

}  // namespace logger::detail
