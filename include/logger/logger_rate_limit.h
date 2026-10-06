#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>

namespace logger::detail
{

inline bool allowOnce(std::atomic<bool>& state) noexcept
{
    bool expected = false;
    return state.compare_exchange_strong(expected, true, std::memory_order_relaxed,
                                         std::memory_order_relaxed);
}

inline bool allowEveryN(std::atomic<std::uint64_t>& count, std::uint64_t interval) noexcept
{
    if (interval == 0)
    {
        return false;
    }
    const std::uint64_t attempt = count.fetch_add(1, std::memory_order_relaxed);
    return attempt == 0 || attempt % interval == 0;
}

template <typename Rep, typename Period>
inline bool allowThrottle(
    std::atomic<std::int64_t>& next_allowed,
    std::chrono::duration<Rep, Period> interval) noexcept
{
    if (interval <= interval.zero())
    {
        return true;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            now.time_since_epoch())
                            .count();
    const auto interval_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count();
    if (interval_ns <= 0)
    {
        return true;
    }

    std::int64_t expected = next_allowed.load(std::memory_order_relaxed);
    while (now_ns >= expected)
    {
        const auto max_value = std::numeric_limits<std::int64_t>::max();
        const std::int64_t desired =
            interval_ns > max_value - now_ns ? max_value : now_ns + interval_ns;
        if (next_allowed.compare_exchange_weak(expected, desired, std::memory_order_relaxed,
                                               std::memory_order_relaxed))
        {
            return true;
        }
    }
    return false;
}

}  // namespace logger::detail
