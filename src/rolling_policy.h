#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "config.h"

namespace logger::detail
{

using Clock = std::function<std::chrono::system_clock::time_point()>;

class RollingError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class IRollingPolicy
{
public:
    virtual ~IRollingPolicy() = default;
    virtual bool shouldRoll(std::uint64_t current_size, std::uint64_t incoming_size,
                            std::chrono::system_clock::time_point now,
                            const std::string& active_period) const = 0;
    virtual bool isTimeBased() const noexcept = 0;
    virtual std::string periodKey(std::chrono::system_clock::time_point time) const = 0;
    virtual std::size_t periodKeyLength() const noexcept = 0;
};

std::unique_ptr<IRollingPolicy> createRollingPolicy(const SinkConfig& config);

}  // namespace logger::detail
