#include "rolling_policy.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace logger::detail
{

namespace
{

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

std::string formatPeriod(std::chrono::system_clock::time_point time, bool hourly)
{
    const std::time_t calendar_time = std::chrono::system_clock::to_time_t(time);
    const std::tm calendar = localTime(calendar_time);
    std::ostringstream output;
    output << std::put_time(&calendar, hourly ? "%Y-%m-%d-%H" : "%Y-%m-%d");
    return output.str();
}

class NoRollingPolicy final : public IRollingPolicy
{
public:
    bool shouldRoll(std::uint64_t, std::uint64_t, std::chrono::system_clock::time_point,
                    const std::string&) const override
    {
        return false;
    }

    bool isTimeBased() const noexcept override { return false; }
    std::string periodKey(std::chrono::system_clock::time_point) const override { return {}; }
    std::size_t periodKeyLength() const noexcept override { return 0; }
};

class SizeRollingPolicy final : public IRollingPolicy
{
public:
    explicit SizeRollingPolicy(std::uint64_t max_size) : max_size_(max_size) {}

    bool shouldRoll(std::uint64_t current_size, std::uint64_t incoming_size,
                    std::chrono::system_clock::time_point,
                    const std::string&) const override
    {
        return current_size != 0 &&
               (current_size > max_size_ || incoming_size > max_size_ - current_size);
    }

    bool isTimeBased() const noexcept override { return false; }
    std::string periodKey(std::chrono::system_clock::time_point) const override { return {}; }
    std::size_t periodKeyLength() const noexcept override { return 0; }

private:
    std::uint64_t max_size_;
};

class TimeRollingPolicy final : public IRollingPolicy
{
public:
    explicit TimeRollingPolicy(bool hourly) : hourly_(hourly) {}

    bool shouldRoll(std::uint64_t current_size, std::uint64_t,
                    std::chrono::system_clock::time_point now,
                    const std::string& active_period) const override
    {
        return current_size != 0 && !active_period.empty() && active_period != periodKey(now);
    }

    bool isTimeBased() const noexcept override { return true; }
    std::string periodKey(std::chrono::system_clock::time_point time) const override
    {
        return formatPeriod(time, hourly_);
    }
    std::size_t periodKeyLength() const noexcept override { return hourly_ ? 13 : 10; }

private:
    bool hourly_;
};

class SizeAndDailyRollingPolicy final : public IRollingPolicy
{
public:
    explicit SizeAndDailyRollingPolicy(std::uint64_t max_size) : max_size_(max_size) {}

    bool shouldRoll(std::uint64_t current_size, std::uint64_t incoming_size,
                    std::chrono::system_clock::time_point now,
                    const std::string& active_period) const override
    {
        const bool size_limit =
            current_size != 0 &&
            (current_size > max_size_ || incoming_size > max_size_ - current_size);
        const bool day_changed =
            current_size != 0 && !active_period.empty() && active_period != periodKey(now);
        return size_limit || day_changed;
    }

    bool isTimeBased() const noexcept override { return true; }
    std::string periodKey(std::chrono::system_clock::time_point time) const override
    {
        return formatPeriod(time, false);
    }
    std::size_t periodKeyLength() const noexcept override { return 10; }

private:
    std::uint64_t max_size_;
};

}  // namespace

std::unique_ptr<IRollingPolicy> createRollingPolicy(const SinkConfig& config)
{
    switch (config.file_roll_policy)
    {
        case ERollPolicy::kNone:
            return std::make_unique<NoRollingPolicy>();
        case ERollPolicy::kSize:
            return std::make_unique<SizeRollingPolicy>(config.file_max_size);
        case ERollPolicy::kDaily:
            return std::make_unique<TimeRollingPolicy>(false);
        case ERollPolicy::kHourly:
            return std::make_unique<TimeRollingPolicy>(true);
        case ERollPolicy::kSizeAndDaily:
            return std::make_unique<SizeAndDailyRollingPolicy>(config.file_max_size);
    }
    throw std::invalid_argument("unknown file rolling policy");
}

}  // namespace logger::detail
