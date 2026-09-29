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

std::string escapeContextValue(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value)
    {
        switch (character)
        {
            case '\\':
                escaped += "\\\\";
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
            case ']':
                escaped += "\\]";
                break;
            default:
                escaped.push_back(character);
                break;
        }
    }
    return escaped;
}

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
        : path_(config.file_path), max_size_(config.file_max_size), max_backups_(config.file_max_backups)
    {
        const auto parent = path_.parent_path();
        if (!parent.empty())
        {
            std::filesystem::create_directories(parent);
        }

        auto mode = std::ios::out | std::ios::binary;
        mode |= config.file_append ? std::ios::app : std::ios::trunc;
        output_.open(path_, mode);
        if (!output_)
        {
            throw std::runtime_error("cannot open file '" + config.file_path + "'");
        }

        if (config.file_append && std::filesystem::exists(path_))
        {
            bytes_written_ = std::filesystem::file_size(path_);
        }
    }

    void write(const std::string& line) override
    {
        const std::uint64_t record_size = static_cast<std::uint64_t>(line.size()) + 1;
        if (max_size_ != 0 && bytes_written_ != 0 &&
            (bytes_written_ > max_size_ || record_size > max_size_ - bytes_written_))
        {
            roll();
        }
        output_ << line << '\n';
        if (!output_)
        {
            throw std::runtime_error("file write failed");
        }
        bytes_written_ += record_size;
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
    void roll()
    {
        if (max_backups_ == 0)
        {
            throw std::runtime_error("file rolling requires at least one backup");
        }

        flush();
        output_.close();
        if (output_.fail())
        {
            throw std::runtime_error("file close failed before rolling");
        }

        for (std::size_t backup = max_backups_; backup > 1; --backup)
        {
            const std::filesystem::path source = path_.string() + "." + std::to_string(backup - 1);
            const std::filesystem::path target = path_.string() + "." + std::to_string(backup);
            if (std::filesystem::exists(target))
            {
                std::filesystem::remove(target);
            }
            if (std::filesystem::exists(source))
            {
                std::filesystem::rename(source, target);
            }
        }

        const std::filesystem::path first_backup = path_.string() + ".1";
        if (std::filesystem::exists(first_backup))
        {
            std::filesystem::remove(first_backup);
        }
        if (std::filesystem::exists(path_))
        {
            std::filesystem::rename(path_, first_backup);
        }

        output_.open(path_, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!output_)
        {
            throw std::runtime_error("cannot reopen file after rolling");
        }
        bytes_written_ = 0;
    }

    std::ofstream output_;
    std::filesystem::path path_;
    std::uint64_t max_size_{0};
    std::size_t max_backups_{0};
    std::uint64_t bytes_written_{0};
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
    for (const auto& field : record.context)
    {
        output << " [" << field.key << '=' << escapeContextValue(field.value) << ']';
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
