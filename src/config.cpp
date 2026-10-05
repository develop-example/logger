#include "config.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <unordered_set>

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

std::string lowercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
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

bool parseUnsigned(const std::string& value, std::uint64_t& result)
{
    const std::string normalized = trim(value);
    if (normalized.empty())
    {
        return false;
    }

    std::uint64_t parsed = 0;
    for (const unsigned char character : normalized)
    {
        if (!std::isdigit(character))
        {
            return false;
        }
        const auto digit = static_cast<std::uint64_t>(character - '0');
        if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
        {
            return false;
        }
        parsed = parsed * 10 + digit;
    }
    result = parsed;
    return true;
}

bool parseFileSize(const std::string& value, std::uint64_t& size)
{
    const std::string normalized = trim(value);
    std::size_t number_end = 0;
    while (number_end < normalized.size() && std::isdigit(static_cast<unsigned char>(normalized[number_end])))
    {
        ++number_end;
    }
    if (number_end == 0)
    {
        return false;
    }

    std::uint64_t number = 0;
    if (!parseUnsigned(normalized.substr(0, number_end), number))
    {
        return false;
    }

    const std::string unit = uppercase(trim(normalized.substr(number_end)));
    std::uint64_t multiplier = 1;
    if (unit == "" || unit == "B")
    {
        multiplier = 1;
    }
    else if (unit == "KB")
    {
        multiplier = 1024;
    }
    else if (unit == "MB")
    {
        multiplier = 1024 * 1024;
    }
    else if (unit == "GB")
    {
        multiplier = 1024 * 1024 * 1024;
    }
    else
    {
        return false;
    }

    if (number > std::numeric_limits<std::uint64_t>::max() / multiplier)
    {
        return false;
    }
    size = number * multiplier;
    return true;
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
    bool sinks_configured = false;
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

        if (key == "logger.sinks")
        {
            parsed.sink_config.sinks.clear();
            std::unordered_set<std::string> unique_sinks;
            std::stringstream sink_list(value);
            std::string sink;
            while (std::getline(sink_list, sink, ','))
            {
                sink = lowercase(trim(std::move(sink)));
                if (sink != "console" && sink != "file")
                {
                    setError(error, line_number, "unknown sink '" + sink + "'");
                    return false;
                }
                if (!unique_sinks.insert(sink).second)
                {
                    setError(error, line_number, "duplicate sink '" + sink + "'");
                    return false;
                }
                parsed.sink_config.sinks.push_back(std::move(sink));
            }
            if (parsed.sink_config.sinks.empty())
            {
                setError(error, line_number, "at least one sink is required");
                return false;
            }
            sinks_configured = true;
            continue;
        }

        if (key == "logger.file.path")
        {
            parsed.sink_config.file_path = value;
            continue;
        }

        if (key == "logger.file.append")
        {
            const std::string normalized = lowercase(value);
            if (normalized == "true")
            {
                parsed.sink_config.file_append = true;
            }
            else if (normalized == "false")
            {
                parsed.sink_config.file_append = false;
            }
            else
            {
                setError(error, line_number, "logger.file.append must be true or false");
                return false;
            }
            continue;
        }

        if (key == "logger.file.max_size")
        {
            if (!parseFileSize(value, parsed.sink_config.file_max_size))
            {
                setError(error, line_number,
                         "logger.file.max_size must be a non-negative size with B, KB, MB, or GB unit");
                return false;
            }
            continue;
        }

        if (key == "logger.file.max_backups")
        {
            std::uint64_t max_backups = 0;
            if (!parseUnsigned(value, max_backups) ||
                max_backups > std::numeric_limits<std::size_t>::max())
            {
                setError(error, line_number, "logger.file.max_backups must be a non-negative integer");
                return false;
            }
            parsed.sink_config.file_max_backups = static_cast<std::size_t>(max_backups);
            continue;
        }

        if (key == "logger.file.roll_policy")
        {
            const std::string policy = lowercase(value);
            if (policy == "none")
            {
                parsed.sink_config.file_roll_policy = ERollPolicy::kNone;
            }
            else if (policy == "size")
            {
                parsed.sink_config.file_roll_policy = ERollPolicy::kSize;
            }
            else if (policy == "daily")
            {
                parsed.sink_config.file_roll_policy = ERollPolicy::kDaily;
            }
            else if (policy == "hourly")
            {
                parsed.sink_config.file_roll_policy = ERollPolicy::kHourly;
            }
            else if (policy == "size_and_daily")
            {
                parsed.sink_config.file_roll_policy = ERollPolicy::kSizeAndDaily;
            }
            else
            {
                setError(error, line_number,
                         "logger.file.roll_policy must be none, size, daily, hourly, or size_and_daily");
                return false;
            }
            parsed.sink_config.file_roll_policy_configured = true;
            continue;
        }

        if (key == "logger.file.max_age_days")
        {
            std::uint64_t max_age_days = 0;
            if (!parseUnsigned(value, max_age_days) ||
                max_age_days > std::numeric_limits<std::size_t>::max())
            {
                setError(error, line_number, "logger.file.max_age_days must be a non-negative integer");
                return false;
            }
            parsed.sink_config.file_max_age_days = static_cast<std::size_t>(max_age_days);
            continue;
        }

        if (key == "logger.format")
        {
            const std::string format = lowercase(value);
            if (format == "text")
            {
                parsed.formatter_config.format = ELogFormat::kText;
            }
            else if (format == "json")
            {
                parsed.formatter_config.format = ELogFormat::kJson;
            }
            else
            {
                setError(error, line_number, "logger.format must be text or json");
                return false;
            }
            continue;
        }

        if (key == "logger.pattern")
        {
            parsed.formatter_config.pattern = value;
            parsed.formatter_config.pattern_configured = true;
            continue;
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

    if (!sinks_configured)
    {
        parsed.sink_config.sinks = {"console"};
    }
    const bool file_enabled =
        std::find(parsed.sink_config.sinks.begin(), parsed.sink_config.sinks.end(), "file") !=
        parsed.sink_config.sinks.end();
    if (!parsed.sink_config.file_roll_policy_configured && parsed.sink_config.file_max_size != 0)
    {
        parsed.sink_config.file_roll_policy = ERollPolicy::kSize;
    }
    if (std::find(parsed.sink_config.sinks.begin(), parsed.sink_config.sinks.end(), "file") !=
            parsed.sink_config.sinks.end() &&
        parsed.sink_config.file_path.empty())
    {
        error = "logger.file.path is required when the file sink is enabled";
        return false;
    }
    if (parsed.sink_config.file_roll_policy != ERollPolicy::kNone && !file_enabled)
    {
        error = "a file sink is required when file rolling is enabled";
        return false;
    }
    if ((parsed.sink_config.file_roll_policy == ERollPolicy::kSize ||
         parsed.sink_config.file_roll_policy == ERollPolicy::kSizeAndDaily) &&
        parsed.sink_config.file_max_size == 0)
    {
        error = "logger.file.max_size must be greater than zero for the selected rolling policy";
        return false;
    }
    if (parsed.sink_config.file_roll_policy != ERollPolicy::kNone &&
        parsed.sink_config.file_max_backups == 0)
    {
        error = "logger.file.max_backups must be greater than zero when file rolling is enabled";
        return false;
    }
    if (parsed.formatter_config.format == ELogFormat::kJson &&
        parsed.formatter_config.pattern_configured)
    {
        error = "logger.pattern cannot be used when logger.format=json";
        return false;
    }

    config = std::move(parsed);
    return true;
}

}  // namespace logger::detail
