#include "sink.h"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace logger::detail
{

namespace
{

class StreamSink : public ILogSink
{
public:
    explicit StreamSink(std::ostream& output) : output_(output) {}

    void write(const std::string& line) override
    {
        output_ << line << '\n';
        if (!output_)
        {
            throw std::runtime_error("stream write failed");
        }
    }

    void flush() override
    {
        output_.flush();
        if (!output_)
        {
            throw std::runtime_error("stream flush failed");
        }
    }

    void close() override { flush(); }

private:
    std::ostream& output_;
};

class ConsoleSink final : public StreamSink
{
public:
    ConsoleSink() : StreamSink(std::clog) {}
};

class FileSink final : public ILogSink
{
public:
    explicit FileSink(const SinkConfig& config)
    {
        const std::filesystem::path path(config.file_path);
        const auto parent = path.parent_path();
        if (!parent.empty())
        {
            std::filesystem::create_directories(parent);
        }

        auto mode = std::ios::out | std::ios::binary;
        mode |= config.file_append ? std::ios::app : std::ios::trunc;
        output_.open(path, mode);
        if (!output_)
        {
            throw std::runtime_error("cannot open file '" + config.file_path + "'");
        }
    }

    void write(const std::string& line) override
    {
        output_ << line << '\n';
        if (!output_)
        {
            throw std::runtime_error("file write failed");
        }
    }

    void flush() override
    {
        if (!output_.is_open())
        {
            return;
        }
        output_.flush();
        if (!output_)
        {
            throw std::runtime_error("file flush failed");
        }
    }

    void close() override
    {
        if (!output_.is_open())
        {
            return;
        }
        flush();
        output_.close();
        if (output_.fail())
        {
            throw std::runtime_error("file close failed");
        }
    }

private:
    std::ofstream output_;
};

std::tm localTime(std::time_t time)
{
    std::tm result{};
#if defined(_WIN32)
    localtime_s(&result, &time);
#else
    localtime_r(&time, &result);
#endif
    return result;
}

}  // namespace

std::string formatRecord(const LogRecord& record)
{
    std::ostringstream output;
    const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(record.timestamp);
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(record.timestamp - seconds).count();
    const std::time_t time = std::chrono::system_clock::to_time_t(record.timestamp);
    const std::tm calendar = localTime(time);

    output << std::put_time(&calendar, "%Y-%m-%d %H:%M:%S") << '.'
           << std::setfill('0') << std::setw(3) << milliseconds << std::setfill(' ')
           << " [" << toString(record.level) << "]"
           << " [tid=" << record.thread_id << "]";
    if (!record.name.empty())
    {
        output << " [" << record.name << ']';
    }
    if (!record.file.empty())
    {
        output << " [" << record.file << ':' << record.line << ']';
    }
    if (!record.function.empty())
    {
        output << " [" << record.function << ']';
    }
    output << ' ' << record.message;
    return output.str();
}

bool createSinks(const SinkConfig& config, std::vector<std::unique_ptr<ILogSink>>& sinks,
                 std::string& error)
{
    std::vector<std::unique_ptr<ILogSink>> created;
    try
    {
        for (const auto& name : config.sinks)
        {
            if (name == "console")
            {
                created.push_back(std::make_unique<ConsoleSink>());
            }
            else if (name == "file")
            {
                created.push_back(std::make_unique<FileSink>(config));
            }
        }
    }
    catch (const std::exception& exception)
    {
        error = exception.what();
        return false;
    }

    if (created.empty())
    {
        error = "no output sinks configured";
        return false;
    }
    sinks = std::move(created);
    return true;
}

std::unique_ptr<ILogSink> createStreamSink(std::ostream& output)
{
    return std::make_unique<StreamSink>(output);
}

}  // namespace logger::detail
