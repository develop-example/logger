#include "config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace logger::detail
{

namespace
{

std::string trim(std::string value)
{
    const auto first = std::find_if_not(value.begin(), value.end(),
                                        [](unsigned char character) { return std::isspace(character); });
    value.erase(value.begin(), first);

    const auto last = std::find_if_not(value.rbegin(), value.rend(),
                                       [](unsigned char character) { return std::isspace(character); });
    value.erase(last.base(), value.end());
    return value;
}

std::string uppercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return value;
}

bool parseLevel(const std::string& value, ELogLevel& level)
{
    const std::string normalized = uppercase(trim(value));
    if (normalized == "DEBUG")
    {
        level = ELogLevel::kDebug;
        return true;
    }
    if (normalized == "INFO")
    {
        level = ELogLevel::kInfo;
        return true;
    }
    if (normalized == "WARN" || normalized == "WARNING")
    {
        level = ELogLevel::kWarn;
        return true;
    }
    if (normalized == "ERROR")
    {
        level = ELogLevel::kError;
        return true;
    }
    if (normalized == "FATAL")
    {
        level = ELogLevel::kFatal;
        return true;
    }
    return false;
}

void setError(std::string& error, std::size_t line, const std::string& reason)
{
    std::ostringstream stream;
    stream << "line " << line << ": " << reason;
    error = stream.str();
}

}  // namespace

bool parseConfigFile(const std::string& path, LoggerConfig& config, std::string& error)
{
    std::ifstream input(path);
    if (!input)
    {
        error = "cannot open file '" + path + "'";
        return false;
    }

    LoggerConfig parsed;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line))
    {
        ++line_number;
        const auto comment = line.find('#');
        if (comment != std::string::npos)
        {
            line.erase(comment);
        }
        line = trim(std::move(line));
        if (line.empty())
        {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos)
        {
            setError(error, line_number, "expected key=value");
            return false;
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        if (key.empty() || value.empty())
        {
            setError(error, line_number, "key and value must not be empty");
            return false;
        }

        std::string module;
        if (key == "logger.level")
        {
            // Root level is intentionally handled below.
        }
        else if (key.size() > 13 && key.compare(0, 7, "logger.") == 0 &&
                 key.compare(key.size() - 6, 6, ".level") == 0)
        {
            module = key.substr(7, key.size() - 13);
            if (module.empty())
            {
                setError(error, line_number, "module name must not be empty");
                return false;
            }
        }
        else
        {
            setError(error, line_number, "unknown property '" + key + "'");
            return false;
        }

        ELogLevel level;
        if (!parseLevel(value, level))
        {
            setError(error, line_number, "invalid log level '" + value + "'");
            return false;
        }

        if (module.empty())
        {
            parsed.root_level = level;
        }
        else
        {
            // Duplicate keys use the last value, matching common properties
            // file behavior and making overrides explicit.
            parsed.module_levels[module] = level;
        }
    }

    if (input.bad())
    {
        error = "failed while reading file '" + path + "'";
        return false;
    }

    config = std::move(parsed);
    return true;
}

}  // namespace logger::detail
