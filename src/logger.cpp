#include "logger/logger.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <utility>

namespace logger
{

namespace
{

class Logger final : public ILogger
{
public:
    void print(ELogLevel level, const char* name, const char* file,
               std::uint32_t line, const char* function,
               const char* format, ...) override
    {
        if (format == nullptr || !shouldLog(level))
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
        if (!shouldLog(level))
        {
            return;
        }

        write(makeRecord(level, name, file, line, function, stream.str()));
    }

    void setLogLevel(ELogLevel level) noexcept override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;
    }

    ELogLevel getLogLevel() const noexcept override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return level_;
    }

    bool shouldLog(ELogLevel level) const noexcept override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return static_cast<unsigned>(level) >= static_cast<unsigned>(level_);
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
        if (static_cast<unsigned>(record.level) < static_cast<unsigned>(level_))
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
    ELogLevel level_{ELogLevel::kDebug};
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
    return "0.2.0";
}

}  // namespace logger
