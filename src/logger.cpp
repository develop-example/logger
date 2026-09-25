#include "logger/logger.h"
#include "config.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <utility>
#include <cstdlib>

namespace logger
{

namespace
{

class Logger final : public ILogger
{
public:
    Logger()
    {
        const char* environment_path = std::getenv("LOGGER_CONFIG_FILE");
        if (environment_path != nullptr && *environment_path != '\0')
        {
            loadConfig(environment_path);
        }
    }

    void print(ELogLevel level, const char* name, const char* file,
               std::uint32_t line, const char* function,
               const char* format, ...) override
    {
        if (format == nullptr || !shouldLog(name, level))
        {
            return;
        }

        va_list first_args;
        va_start(first_args, format);
        va_list sizing_args;
        va_copy(sizing_args, first_args);
        const int length = std::vsnprintf(nullptr, 0, format, sizing_args);
        va_end(sizing_args);

        if (length < 0)
        {
            va_end(first_args);
            return;
        }

        std::string buffer(static_cast<std::size_t>(length) + 1, '\0');
        va_list writing_args;
        va_copy(writing_args, first_args);
        std::vsnprintf(buffer.data(), buffer.size(), format, writing_args);
        va_end(writing_args);
        va_end(first_args);

        buffer.resize(static_cast<std::size_t>(length));
        write(makeRecord(level, name, file, line, function, std::move(buffer)));
    }

    void print(ELogLevel level, const char* name, const char* file,
               std::uint32_t line, const char* function,
               const std::stringstream& stream) override
    {
        if (!shouldLog(name, level))
        {
            return;
        }

        write(makeRecord(level, name, file, line, function, stream.str()));
    }

    void setLogLevel(ELogLevel level) noexcept override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.root_level = level;
    }

    void setLogLevel(const std::string& name, ELogLevel level) noexcept override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (name.empty())
        {
            config_.root_level = level;
        }
        else
        {
            config_.module_levels[name] = level;
        }
    }

    ELogLevel getLogLevel() const noexcept override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_.root_level;
    }

    ELogLevel getLogLevel(const std::string& name) const noexcept override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return resolveLevelUnlocked(name);
    }

    bool shouldLog(ELogLevel level) const noexcept override
    {
        return shouldLog(nullptr, level);
    }

    bool shouldLog(const char* name, ELogLevel level) const noexcept override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return static_cast<unsigned>(level) >=
               static_cast<unsigned>(resolveLevelUnlocked(name != nullptr ? name : ""));
    }

    bool loadConfig(const std::string& path) override
    {
        detail::LoggerConfig parsed;
        std::string error;
        if (!detail::parseConfigFile(path, parsed, error))
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::cerr << "logger: failed to load config '" << path << "': " << error << '\n';
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            config_ = std::move(parsed);
            config_path_ = path;
        }
        return true;
    }

    bool reloadConfig() override
    {
        const std::string path = getConfigPath();
        return !path.empty() && loadConfig(path);
    }

    std::string getConfigPath() const override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_path_;
    }

    void setOutput(std::ostream& output) noexcept override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        output_ = &output;
    }

    void resetOutput() noexcept override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        output_ = &std::clog;
    }

    void flush() noexcept override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        output_->flush();
    }

private:
    static LogRecord makeRecord(ELogLevel level, const char* name, const char* file,
                                std::uint32_t line, const char* function,
                                std::string message)
    {
        LogRecord record;
        record.timestamp = std::chrono::system_clock::now();
        record.level = level;
        record.name = name != nullptr ? name : "";
        record.file = file != nullptr ? file : "";
        record.line = line;
        record.function = function != nullptr ? function : "";
        record.thread_id = std::this_thread::get_id();
        record.message = std::move(message);
        return record;
    }

    static std::tm localTime(std::time_t time)
    {
        std::tm result{};
#if defined(_WIN32)
        localtime_s(&result, &time);
#else
        localtime_r(&time, &result);
#endif
        return result;
    }

    static void writeTimestamp(std::ostream& output,
                               std::chrono::system_clock::time_point timestamp)
    {
        const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(timestamp);
        const auto milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - seconds).count();
        const std::time_t time = std::chrono::system_clock::to_time_t(timestamp);
        const std::tm calendar = localTime(time);

        output << std::put_time(&calendar, "%Y-%m-%d %H:%M:%S") << '.'
               << std::setfill('0') << std::setw(3) << milliseconds << std::setfill(' ');
    }

    void write(const LogRecord& record)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Re-check the threshold after formatting in case it changed while
        // this call was preparing its record.
        if (static_cast<unsigned>(record.level) <
            static_cast<unsigned>(resolveLevelUnlocked(record.name)))
        {
            return;
        }

        writeTimestamp(*output_, record.timestamp);
        *output_ << " [" << toString(record.level) << "]"
                 << " [tid=" << record.thread_id << "]";
        if (!record.name.empty())
        {
            *output_ << " [" << record.name << ']';
        }
        if (!record.file.empty())
        {
            *output_ << " [" << record.file << ':' << record.line << ']';
        }
        if (!record.function.empty())
        {
            *output_ << " [" << record.function << ']';
        }
        *output_ << ' ' << record.message << '\n';
    }

    mutable std::mutex mutex_;
    ELogLevel resolveLevelUnlocked(const std::string& name) const noexcept
    {
        std::string candidate = name;
        while (!candidate.empty())
        {
            const auto found = config_.module_levels.find(candidate);
            if (found != config_.module_levels.end())
            {
                return found->second;
            }

            const auto separator = candidate.rfind('.');
            if (separator == std::string::npos)
            {
                break;
            }
            candidate.erase(separator);
        }
        return config_.root_level;
    }

    detail::LoggerConfig config_;
    std::string config_path_;
    std::ostream* output_{&std::clog};
};

}  // namespace

ILogger* ILogger::getInstance() noexcept
{
    static Logger instance;
    return &instance;
}

const char* toString(ELogLevel level) noexcept
{
    switch (level)
    {
        case ELogLevel::kDebug:
            return "DEBUG";
        case ELogLevel::kInfo:
            return "INFO";
        case ELogLevel::kWarn:
            return "WARN";
        case ELogLevel::kError:
            return "ERROR";
        case ELogLevel::kFatal:
            return "FATAL";
    }
    return "UNKNOWN";
}

const char* version() noexcept
{
    return "0.4.0";
}

}  // namespace logger
