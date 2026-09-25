#pragma once

#include <string>
#include <unordered_map>

#include "logger/logger_api.h"

namespace logger::detail
{

struct LoggerConfig
{
    ELogLevel root_level{ELogLevel::kDebug};
    std::unordered_map<std::string, ELogLevel> module_levels;
};

bool parseConfigFile(const std::string& path, LoggerConfig& config, std::string& error);

}  // namespace logger::detail
