#include "logger/logger.h"
#include "config.h"
#include "sink.h"

#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

namespace logger
{

namespace
{

class Logger final : public ILogger
{
public:
    Logger()
    {
        std::string sink_error;
        if (!detail::createSinks(config_.sink_config, sinks_, sink_error))
        {
            std::cerr << "logger: failed to create default sinks: " << sink_error << '\n';
        }
        const char* environment_path = std::getenv("LOGGER_CONFIG_FILE");
        if (environment_path != nullptr && *environment_path != '\0')
        {
            loadConfig(environment_path);
        }
        worker_ = std::thread(&Logger::workerLoop, this);
    }

    ~Logger() override
    {
        shutdown();
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
        enqueue(makeRecord(level, name, file, line, function, std::move(buffer)));
    }

    void print(ELogLevel level, const char* name, const char* file,
               std::uint32_t line, const char* function,
               const std::stringstream& stream) override
    {
        if (!shouldLog(name, level))
        {
            return;
        }

        enqueue(makeRecord(level, name, file, line, function, stream.str()));
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

    void shutdown() noexcept override
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            accepting_ = false;
        }
        queue_condition_.notify_all();
        space_condition_.notify_all();

        std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
        if (worker_.joinable() && std::this_thread::get_id() != worker_.get_id())
        {
            worker_.join();
        }

        std::lock_guard<std::mutex> output_lock(output_mutex_);
        flushSinksUnlocked();
        closeActiveSinksUnlocked();
    }

    QueueStats getQueueStats() const noexcept override
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        QueueStats stats;
        stats.capacity = queue_capacity_;
        stats.size = queue_.size();
        stats.peak_size = peak_size_;
        stats.accepted = accepted_;
        stats.dropped = dropped_;
        stats.dropped_debug = dropped_debug_;
        stats.dropped_info = dropped_info_;
        stats.sink_errors = sink_errors_.load(std::memory_order_relaxed);
        return stats;
    }

    bool setQueueCapacity(std::size_t capacity) noexcept override
    {
        if (capacity == 0)
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (!accepting_ || capacity < queue_.size())
        {
            return false;
        }
        queue_capacity_ = capacity;
        space_condition_.notify_all();
        return true;
    }

    void setQueueOverflowPolicy(EQueueOverflowPolicy policy) noexcept override
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        overflow_policy_ = policy;
        space_condition_.notify_all();
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

        std::vector<std::unique_ptr<detail::ILogSink>> new_sinks;
        if (!detail::createSinks(parsed.sink_config, new_sinks, error))
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::cerr << "logger: failed to create sinks for config '" << path << "': "
                      << error << '\n';
            return false;
        }

        std::vector<std::unique_ptr<detail::ILogSink>> old_sinks;
        {
            std::scoped_lock lock(mutex_, output_mutex_);
            config_ = std::move(parsed);
            config_path_ = path;
            old_sinks.swap(sinks_);
            sinks_.swap(new_sinks);
        }
        closeSinks(old_sinks);
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
        std::lock_guard<std::mutex> lock(output_mutex_);
        try
        {
            if (custom_output_)
            {
                custom_output_->close();
            }
            custom_output_ = detail::createStreamSink(output);
        }
        catch (...)
        {
            sink_errors_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void resetOutput() noexcept override
    {
        std::lock_guard<std::mutex> lock(output_mutex_);
        if (custom_output_)
        {
            try
            {
                custom_output_->close();
            }
            catch (...)
            {
                sink_errors_.fetch_add(1, std::memory_order_relaxed);
            }
            custom_output_.reset();
        }
    }

    void flush() noexcept override
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        drain_condition_.wait(lock, [this] { return queue_.empty() && active_count_ == 0; });
        lock.unlock();

        std::lock_guard<std::mutex> output_lock(output_mutex_);
        flushSinksUnlocked();
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

    void enqueue(LogRecord record)
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (!accepting_)
        {
            recordDropUnlocked(record.level);
            return;
        }

        while (queue_.size() >= queue_capacity_)
        {
            if (overflow_policy_ == EQueueOverflowPolicy::kDropNewest ||
                (overflow_policy_ == EQueueOverflowPolicy::kDropLowPriority &&
                 isLowPriority(record.level)))
            {
                recordDropUnlocked(record.level);
                return;
            }

            space_condition_.wait(lock, [this] { return !accepting_ || queue_.size() < queue_capacity_; });
            if (!accepting_)
            {
                recordDropUnlocked(record.level);
                return;
            }
        }

        queue_.push_back(std::move(record));
        ++accepted_;
        peak_size_ = std::max(peak_size_, queue_.size());
        lock.unlock();
        queue_condition_.notify_one();
    }

    static bool isLowPriority(ELogLevel level) noexcept
    {
        return level == ELogLevel::kDebug || level == ELogLevel::kInfo;
    }

    void recordDropUnlocked(ELogLevel level) noexcept
    {
        ++dropped_;
        if (level == ELogLevel::kDebug)
        {
            ++dropped_debug_;
        }
        else if (level == ELogLevel::kInfo)
        {
            ++dropped_info_;
        }
    }

    void workerLoop() noexcept
    {
        while (true)
        {
            LogRecord record;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_condition_.wait(lock, [this] { return !queue_.empty() || !accepting_; });
                if (queue_.empty() && !accepting_)
                {
                    break;
                }
                record = std::move(queue_.front());
                queue_.pop_front();
                ++active_count_;
                space_condition_.notify_one();
            }

            try
            {
                write(record);
            }
            catch (...)
            {
                // A failing output stream must not terminate the worker.
            }

            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                --active_count_;
                if (queue_.empty() && active_count_ == 0)
                {
                    drain_condition_.notify_all();
                }
            }
        }

        std::lock_guard<std::mutex> lock(queue_mutex_);
        drain_condition_.notify_all();
    }

    void write(const LogRecord& record)
    {
        std::lock_guard<std::mutex> lock(output_mutex_);
        const std::string line = detail::formatRecord(record);
        if (custom_output_)
        {
            writeToSink(*custom_output_, line);
            return;
        }
        for (const auto& sink : sinks_)
        {
            writeToSink(*sink, line);
        }
    }

    void writeToSink(detail::ILogSink& sink, const std::string& line) noexcept
    {
        try
        {
            sink.write(line);
        }
        catch (...)
        {
            sink_errors_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void flushSinksUnlocked() noexcept
    {
        if (custom_output_)
        {
            try
            {
                custom_output_->flush();
            }
            catch (...)
            {
                sink_errors_.fetch_add(1, std::memory_order_relaxed);
            }
            return;
        }
        for (const auto& sink : sinks_)
        {
            try
            {
                sink->flush();
            }
            catch (...)
            {
                sink_errors_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    void closeSinks(std::vector<std::unique_ptr<detail::ILogSink>>& sinks) noexcept
    {
        for (auto& sink : sinks)
        {
            try
            {
                sink->close();
            }
            catch (...)
            {
                sink_errors_.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }

    void closeActiveSinksUnlocked() noexcept
    {
        if (custom_output_)
        {
            try
            {
                custom_output_->close();
            }
            catch (...)
            {
                sink_errors_.fetch_add(1, std::memory_order_relaxed);
            }
            custom_output_.reset();
        }
        closeSinks(sinks_);
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

    mutable std::mutex output_mutex_;
    std::vector<std::unique_ptr<detail::ILogSink>> sinks_;
    std::unique_ptr<detail::ILogSink> custom_output_;
    std::atomic<std::uint64_t> sink_errors_{0};
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    std::condition_variable space_condition_;
    std::condition_variable drain_condition_;
    std::deque<LogRecord> queue_;
    std::size_t queue_capacity_{8192};
    std::size_t peak_size_{0};
    std::size_t active_count_{0};
    std::uint64_t accepted_{0};
    std::uint64_t dropped_{0};
    std::uint64_t dropped_debug_{0};
    std::uint64_t dropped_info_{0};
    EQueueOverflowPolicy overflow_policy_{EQueueOverflowPolicy::kDropLowPriority};
    bool accepting_{true};
    std::thread worker_;
    std::mutex lifecycle_mutex_;
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
    return "0.6.0";
}

}  // namespace logger
