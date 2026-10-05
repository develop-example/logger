#include "sink.h"

#include "rolling_policy.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
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
        : path_(config.file_path), max_backups_(config.file_max_backups),
          max_age_days_(config.file_max_age_days), policy_(createRollingPolicy(config)),
          active_period_(policy_->isTimeBased() ? policy_->periodKey(std::chrono::system_clock::now())
                                                 : "")
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
            if (policy_->isTimeBased())
            {
                active_period_ = filePeriod(path_);
            }
        }
    }

    void write(const std::string& line) override
    {
        const std::uint64_t record_size = static_cast<std::uint64_t>(line.size()) + 1;
        const auto now = std::chrono::system_clock::now();
        if (policy_->shouldRoll(bytes_written_, record_size, now, active_period_))
        {
            roll();
        }
        output_ << line << '\n';
        if (!output_)
        {
            throw std::runtime_error("file write failed");
        }
        bytes_written_ += record_size;
        if (policy_->isTimeBased() && active_period_.empty())
        {
            active_period_ = policy_->periodKey(now);
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
    void roll()
    {
        if (max_backups_ == 0)
        {
            throw RollingError("file rolling requires at least one backup");
        }

        try
        {
            flush();
        }
        catch (const std::exception& exception)
        {
            throw RollingError(exception.what());
        }
        output_.close();
        if (output_.fail())
        {
            throw RollingError("file close failed before rolling");
        }

        try
        {
            if (policy_->isTimeBased())
            {
                rollTimeBased();
            }
            else
            {
                rollSizeBased();
            }
        }
        catch (const RollingError&)
        {
            throw;
        }
        catch (const std::exception& exception)
        {
            throw RollingError(exception.what());
        }

        output_.open(path_, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!output_)
        {
            throw RollingError("cannot reopen file after rolling");
        }
        bytes_written_ = 0;
        if (policy_->isTimeBased())
        {
            active_period_ = policy_->periodKey(std::chrono::system_clock::now());
        }
    }

    std::string filePeriod(const std::filesystem::path& path) const
    {
        const auto file_time = std::filesystem::last_write_time(path);
        const auto system_time = std::chrono::system_clock::now() +
                                 (file_time - decltype(file_time)::clock::now());
        return policy_->periodKey(system_time);
    }

    void rollSizeBased()
    {
        for (std::size_t backup = max_backups_; backup > 1; --backup)
        {
            const std::filesystem::path source = path_.string() + "." + std::to_string(backup - 1);
            const std::filesystem::path target = path_.string() + "." + std::to_string(backup);
            if (std::filesystem::exists(target)) std::filesystem::remove(target);
            if (std::filesystem::exists(source)) std::filesystem::rename(source, target);
        }
        const std::filesystem::path first_backup = path_.string() + ".1";
        if (std::filesystem::exists(first_backup)) std::filesystem::remove(first_backup);
        if (std::filesystem::exists(path_)) std::filesystem::rename(path_, first_backup);
    }

    void rollTimeBased()
    {
        const std::string period = active_period_.empty() ? "unknown" : active_period_;
        std::filesystem::path target = path_.string() + "." + period;
        if (std::filesystem::exists(target))
        {
            bool free_slot = false;
            for (std::size_t suffix = 1; suffix <= max_backups_; ++suffix)
            {
                const auto candidate = path_.string() + "." + period + "." + std::to_string(suffix);
                if (!std::filesystem::exists(candidate))
                {
                    target = candidate;
                    free_slot = true;
                    break;
                }
            }
            if (!free_slot)
            {
                target = path_.string() + "." + period + "." + std::to_string(max_backups_);
                std::filesystem::remove(target);
            }
        }
        if (std::filesystem::exists(path_)) std::filesystem::rename(path_, target);
        cleanupTimeBackups();
    }

    void cleanupTimeBackups()
    {
        std::vector<std::filesystem::path> backups;
        const auto parent = path_.parent_path().empty() ? std::filesystem::path(".") : path_.parent_path();
        const std::string prefix = path_.filename().string() + ".";
        for (const auto& entry : std::filesystem::directory_iterator(parent))
        {
            const std::string name = entry.path().filename().string();
            if (entry.is_regular_file() && name.rfind(prefix, 0) == 0 && name.size() >= prefix.size() + 10)
            {
                backups.push_back(entry.path());
            }
        }
        std::sort(backups.begin(), backups.end());
        while (backups.size() > max_backups_)
        {
            std::filesystem::remove(backups.front());
            backups.erase(backups.begin());
        }
        if (max_age_days_ != 0)
        {
            const auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24 * max_age_days_);
            for (const auto& backup : backups)
            {
                const auto file_time = std::filesystem::last_write_time(backup);
                const auto system_time = std::chrono::system_clock::now() +
                                         (file_time - decltype(file_time)::clock::now());
                if (system_time < cutoff) std::filesystem::remove(backup);
            }
        }
    }

    std::ofstream output_;
    std::filesystem::path path_;
    std::size_t max_backups_{0};
    std::size_t max_age_days_{0};
    std::unique_ptr<IRollingPolicy> policy_;
    std::string active_period_;
    std::uint64_t bytes_written_{0};
};

}  // namespace

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
