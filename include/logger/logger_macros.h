#pragma once

#include "logger/logger_api.h"
#include "logger/logger_rate_limit.h"

#if defined(_MSC_VER)
#define LOGGER_DETAIL_FUNCTION __FUNCSIG__
#else
#define LOGGER_DETAIL_FUNCTION __PRETTY_FUNCTION__
#endif

#define LOGGER_DETAIL_LOG(level, name, ...)                                                   \
    do                                                                                         \
    {                                                                                          \
        auto* logger_detail_instance = ::logger::ILogger::getInstance();                       \
        const char* logger_detail_name = name;                                                 \
        if (logger_detail_instance->shouldLog(logger_detail_name, level))                      \
        {                                                                                      \
            logger_detail_instance->print(level, name, __FILE__, __LINE__,                     \
                                          LOGGER_DETAIL_FUNCTION, __VA_ARGS__);                 \
        }                                                                                      \
    } while (false)

#define LOGGER_DETAIL_LOG_STREAM(level, name, expression)                                      \
    do                                                                                         \
    {                                                                                          \
        auto* logger_detail_instance = ::logger::ILogger::getInstance();                       \
        const char* logger_detail_name = name;                                                 \
        if (logger_detail_instance->shouldLog(logger_detail_name, level))                      \
        {                                                                                      \
            std::stringstream logger_detail_stream;                                            \
            logger_detail_stream << expression;                                                \
            logger_detail_instance->print(level, name, __FILE__, __LINE__,                     \
                                          LOGGER_DETAIL_FUNCTION, logger_detail_stream);        \
        }                                                                                      \
    } while (false)

#define LOGGER_DETAIL_LOG_ONCE(level, name, ...)                                                \
    do                                                                                          \
    {                                                                                           \
        auto* logger_detail_instance = ::logger::ILogger::getInstance();                      \
        const char* logger_detail_name = name;                                                \
        if (logger_detail_instance->shouldLog(logger_detail_name, level))                     \
        {                                                                                      \
            static std::atomic<bool> logger_detail_once_state{false};                          \
            if (::logger::detail::allowOnce(logger_detail_once_state))                        \
            {                                                                                  \
                logger_detail_instance->print(level, name, __FILE__, __LINE__,                \
                                              LOGGER_DETAIL_FUNCTION, __VA_ARGS__);            \
            }                                                                                  \
        }                                                                                      \
    } while (false)

#define LOGGER_DETAIL_LOG_EVERY_N(level, name, interval, ...)                                   \
    do                                                                                          \
    {                                                                                           \
        auto* logger_detail_instance = ::logger::ILogger::getInstance();                      \
        const char* logger_detail_name = name;                                                \
        if (logger_detail_instance->shouldLog(logger_detail_name, level))                     \
        {                                                                                      \
            const std::uint64_t logger_detail_interval =                                       \
                static_cast<std::uint64_t>(interval);                                          \
            static std::atomic<std::uint64_t> logger_detail_every_n_count{0};                  \
            if (::logger::detail::allowEveryN(logger_detail_every_n_count,                    \
                                               logger_detail_interval))                        \
            {                                                                                  \
                logger_detail_instance->print(level, name, __FILE__, __LINE__,                \
                                              LOGGER_DETAIL_FUNCTION, __VA_ARGS__);            \
            }                                                                                  \
        }                                                                                      \
    } while (false)

#define LOGGER_DETAIL_LOG_THROTTLE(level, name, interval, ...)                                  \
    do                                                                                          \
    {                                                                                           \
        auto* logger_detail_instance = ::logger::ILogger::getInstance();                      \
        const char* logger_detail_name = name;                                                \
        if (logger_detail_instance->shouldLog(logger_detail_name, level))                     \
        {                                                                                      \
            static std::atomic<std::int64_t> logger_detail_next_allowed{0};                   \
            if (::logger::detail::allowThrottle(logger_detail_next_allowed, interval))        \
            {                                                                                  \
                logger_detail_instance->print(level, name, __FILE__, __LINE__,                \
                                              LOGGER_DETAIL_FUNCTION, __VA_ARGS__);            \
            }                                                                                  \
        }                                                                                      \
    } while (false)

#define LOGGER_DETAIL_LOG_STREAM_ONCE(level, name, expression)                                  \
    do                                                                                          \
    {                                                                                           \
        auto* logger_detail_instance = ::logger::ILogger::getInstance();                      \
        const char* logger_detail_name = name;                                                \
        if (logger_detail_instance->shouldLog(logger_detail_name, level))                     \
        {                                                                                      \
            static std::atomic<bool> logger_detail_once_state{false};                          \
            if (::logger::detail::allowOnce(logger_detail_once_state))                        \
            {                                                                                  \
                std::stringstream logger_detail_stream;                                        \
                logger_detail_stream << expression;                                            \
                logger_detail_instance->print(level, name, __FILE__, __LINE__,                \
                                              LOGGER_DETAIL_FUNCTION, logger_detail_stream);   \
            }                                                                                  \
        }                                                                                      \
    } while (false)

#define LOGGER_DETAIL_LOG_STREAM_EVERY_N(level, name, interval, expression)                     \
    do                                                                                          \
    {                                                                                           \
        auto* logger_detail_instance = ::logger::ILogger::getInstance();                      \
        const char* logger_detail_name = name;                                                \
        if (logger_detail_instance->shouldLog(logger_detail_name, level))                     \
        {                                                                                      \
            const std::uint64_t logger_detail_interval =                                       \
                static_cast<std::uint64_t>(interval);                                          \
            static std::atomic<std::uint64_t> logger_detail_every_n_count{0};                  \
            if (::logger::detail::allowEveryN(logger_detail_every_n_count,                    \
                                               logger_detail_interval))                        \
            {                                                                                  \
                std::stringstream logger_detail_stream;                                        \
                logger_detail_stream << expression;                                            \
                logger_detail_instance->print(level, name, __FILE__, __LINE__,                \
                                              LOGGER_DETAIL_FUNCTION, logger_detail_stream);   \
            }                                                                                  \
        }                                                                                      \
    } while (false)

#define LOGGER_DETAIL_LOG_STREAM_THROTTLE(level, name, interval, expression)                    \
    do                                                                                          \
    {                                                                                           \
        auto* logger_detail_instance = ::logger::ILogger::getInstance();                      \
        const char* logger_detail_name = name;                                                \
        if (logger_detail_instance->shouldLog(logger_detail_name, level))                     \
        {                                                                                      \
            static std::atomic<std::int64_t> logger_detail_next_allowed{0};                   \
            if (::logger::detail::allowThrottle(logger_detail_next_allowed, interval))        \
            {                                                                                  \
                std::stringstream logger_detail_stream;                                        \
                logger_detail_stream << expression;                                            \
                logger_detail_instance->print(level, name, __FILE__, __LINE__,                \
                                              LOGGER_DETAIL_FUNCTION, logger_detail_stream);   \
            }                                                                                  \
        }                                                                                      \
    } while (false)

#define LOGGER_DEBUG(...) LOGGER_DETAIL_LOG(::logger::ELogLevel::kDebug, "", __VA_ARGS__)
#define LOGGER_INFO(...) LOGGER_DETAIL_LOG(::logger::ELogLevel::kInfo, "", __VA_ARGS__)
#define LOGGER_WARN(...) LOGGER_DETAIL_LOG(::logger::ELogLevel::kWarn, "", __VA_ARGS__)
#define LOGGER_ERROR(...) LOGGER_DETAIL_LOG(::logger::ELogLevel::kError, "", __VA_ARGS__)
#define LOGGER_FATAL(...) LOGGER_DETAIL_LOG(::logger::ELogLevel::kFatal, "", __VA_ARGS__)

#define LOGGER_DEBUG_NAMED(name, ...) \
    LOGGER_DETAIL_LOG(::logger::ELogLevel::kDebug, name, __VA_ARGS__)
#define LOGGER_INFO_NAMED(name, ...) LOGGER_DETAIL_LOG(::logger::ELogLevel::kInfo, name, __VA_ARGS__)
#define LOGGER_WARN_NAMED(name, ...) LOGGER_DETAIL_LOG(::logger::ELogLevel::kWarn, name, __VA_ARGS__)
#define LOGGER_ERROR_NAMED(name, ...) \
    LOGGER_DETAIL_LOG(::logger::ELogLevel::kError, name, __VA_ARGS__)
#define LOGGER_FATAL_NAMED(name, ...) \
    LOGGER_DETAIL_LOG(::logger::ELogLevel::kFatal, name, __VA_ARGS__)

#define LOGGER_DEBUG_STREAM(expression) \
    LOGGER_DETAIL_LOG_STREAM(::logger::ELogLevel::kDebug, "", expression)
#define LOGGER_INFO_STREAM(expression) \
    LOGGER_DETAIL_LOG_STREAM(::logger::ELogLevel::kInfo, "", expression)
#define LOGGER_WARN_STREAM(expression) \
    LOGGER_DETAIL_LOG_STREAM(::logger::ELogLevel::kWarn, "", expression)
#define LOGGER_ERROR_STREAM(expression) \
    LOGGER_DETAIL_LOG_STREAM(::logger::ELogLevel::kError, "", expression)
#define LOGGER_FATAL_STREAM(expression) \
    LOGGER_DETAIL_LOG_STREAM(::logger::ELogLevel::kFatal, "", expression)

#define LOGGER_DEBUG_STREAM_NAMED(name, expression) \
    LOGGER_DETAIL_LOG_STREAM(::logger::ELogLevel::kDebug, name, expression)
#define LOGGER_INFO_STREAM_NAMED(name, expression) \
    LOGGER_DETAIL_LOG_STREAM(::logger::ELogLevel::kInfo, name, expression)
#define LOGGER_WARN_STREAM_NAMED(name, expression) \
    LOGGER_DETAIL_LOG_STREAM(::logger::ELogLevel::kWarn, name, expression)
#define LOGGER_ERROR_STREAM_NAMED(name, expression) \
    LOGGER_DETAIL_LOG_STREAM(::logger::ELogLevel::kError, name, expression)
#define LOGGER_FATAL_STREAM_NAMED(name, expression) \
    LOGGER_DETAIL_LOG_STREAM(::logger::ELogLevel::kFatal, name, expression)

#define LOGGER_DEBUG_ONCE(...) LOGGER_DETAIL_LOG_ONCE(::logger::ELogLevel::kDebug, "", __VA_ARGS__)
#define LOGGER_INFO_ONCE(...) LOGGER_DETAIL_LOG_ONCE(::logger::ELogLevel::kInfo, "", __VA_ARGS__)
#define LOGGER_WARN_ONCE(...) LOGGER_DETAIL_LOG_ONCE(::logger::ELogLevel::kWarn, "", __VA_ARGS__)
#define LOGGER_ERROR_ONCE(...) LOGGER_DETAIL_LOG_ONCE(::logger::ELogLevel::kError, "", __VA_ARGS__)
#define LOGGER_FATAL_ONCE(...) LOGGER_DETAIL_LOG_ONCE(::logger::ELogLevel::kFatal, "", __VA_ARGS__)

#define LOGGER_DEBUG_EVERY_N(interval, ...) \
    LOGGER_DETAIL_LOG_EVERY_N(::logger::ELogLevel::kDebug, "", interval, __VA_ARGS__)
#define LOGGER_INFO_EVERY_N(interval, ...) \
    LOGGER_DETAIL_LOG_EVERY_N(::logger::ELogLevel::kInfo, "", interval, __VA_ARGS__)
#define LOGGER_WARN_EVERY_N(interval, ...) \
    LOGGER_DETAIL_LOG_EVERY_N(::logger::ELogLevel::kWarn, "", interval, __VA_ARGS__)
#define LOGGER_ERROR_EVERY_N(interval, ...) \
    LOGGER_DETAIL_LOG_EVERY_N(::logger::ELogLevel::kError, "", interval, __VA_ARGS__)
#define LOGGER_FATAL_EVERY_N(interval, ...) \
    LOGGER_DETAIL_LOG_EVERY_N(::logger::ELogLevel::kFatal, "", interval, __VA_ARGS__)

#define LOGGER_DEBUG_THROTTLE(interval, ...) \
    LOGGER_DETAIL_LOG_THROTTLE(::logger::ELogLevel::kDebug, "", interval, __VA_ARGS__)
#define LOGGER_INFO_THROTTLE(interval, ...) \
    LOGGER_DETAIL_LOG_THROTTLE(::logger::ELogLevel::kInfo, "", interval, __VA_ARGS__)
#define LOGGER_WARN_THROTTLE(interval, ...) \
    LOGGER_DETAIL_LOG_THROTTLE(::logger::ELogLevel::kWarn, "", interval, __VA_ARGS__)
#define LOGGER_ERROR_THROTTLE(interval, ...) \
    LOGGER_DETAIL_LOG_THROTTLE(::logger::ELogLevel::kError, "", interval, __VA_ARGS__)
#define LOGGER_FATAL_THROTTLE(interval, ...) \
    LOGGER_DETAIL_LOG_THROTTLE(::logger::ELogLevel::kFatal, "", interval, __VA_ARGS__)

#define LOGGER_DEBUG_STREAM_ONCE(expression) \
    LOGGER_DETAIL_LOG_STREAM_ONCE(::logger::ELogLevel::kDebug, "", expression)
#define LOGGER_INFO_STREAM_ONCE(expression) \
    LOGGER_DETAIL_LOG_STREAM_ONCE(::logger::ELogLevel::kInfo, "", expression)
#define LOGGER_WARN_STREAM_ONCE(expression) \
    LOGGER_DETAIL_LOG_STREAM_ONCE(::logger::ELogLevel::kWarn, "", expression)
#define LOGGER_ERROR_STREAM_ONCE(expression) \
    LOGGER_DETAIL_LOG_STREAM_ONCE(::logger::ELogLevel::kError, "", expression)
#define LOGGER_FATAL_STREAM_ONCE(expression) \
    LOGGER_DETAIL_LOG_STREAM_ONCE(::logger::ELogLevel::kFatal, "", expression)

#define LOGGER_DEBUG_STREAM_EVERY_N(interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_EVERY_N(::logger::ELogLevel::kDebug, "", interval, expression)
#define LOGGER_INFO_STREAM_EVERY_N(interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_EVERY_N(::logger::ELogLevel::kInfo, "", interval, expression)
#define LOGGER_WARN_STREAM_EVERY_N(interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_EVERY_N(::logger::ELogLevel::kWarn, "", interval, expression)
#define LOGGER_ERROR_STREAM_EVERY_N(interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_EVERY_N(::logger::ELogLevel::kError, "", interval, expression)
#define LOGGER_FATAL_STREAM_EVERY_N(interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_EVERY_N(::logger::ELogLevel::kFatal, "", interval, expression)

#define LOGGER_DEBUG_STREAM_THROTTLE(interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_THROTTLE(::logger::ELogLevel::kDebug, "", interval, expression)
#define LOGGER_INFO_STREAM_THROTTLE(interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_THROTTLE(::logger::ELogLevel::kInfo, "", interval, expression)
#define LOGGER_WARN_STREAM_THROTTLE(interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_THROTTLE(::logger::ELogLevel::kWarn, "", interval, expression)
#define LOGGER_ERROR_STREAM_THROTTLE(interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_THROTTLE(::logger::ELogLevel::kError, "", interval, expression)
#define LOGGER_FATAL_STREAM_THROTTLE(interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_THROTTLE(::logger::ELogLevel::kFatal, "", interval, expression)

#define LOGGER_DETAIL_LOG_STREAM_NAMED_ONCE(level, name, expression) \
    LOGGER_DETAIL_LOG_STREAM_ONCE(level, name, expression)
#define LOGGER_DETAIL_LOG_STREAM_NAMED_EVERY_N(level, name, interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_EVERY_N(level, name, interval, expression)
#define LOGGER_DETAIL_LOG_STREAM_NAMED_THROTTLE(level, name, interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_THROTTLE(level, name, interval, expression)

#define LOGGER_DEBUG_STREAM_ONCE_NAMED(name, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_ONCE(::logger::ELogLevel::kDebug, name, expression)
#define LOGGER_INFO_STREAM_ONCE_NAMED(name, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_ONCE(::logger::ELogLevel::kInfo, name, expression)
#define LOGGER_WARN_STREAM_ONCE_NAMED(name, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_ONCE(::logger::ELogLevel::kWarn, name, expression)
#define LOGGER_ERROR_STREAM_ONCE_NAMED(name, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_ONCE(::logger::ELogLevel::kError, name, expression)
#define LOGGER_FATAL_STREAM_ONCE_NAMED(name, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_ONCE(::logger::ELogLevel::kFatal, name, expression)

#define LOGGER_DEBUG_STREAM_EVERY_N_NAMED(name, interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_EVERY_N(::logger::ELogLevel::kDebug, name, interval, expression)
#define LOGGER_INFO_STREAM_EVERY_N_NAMED(name, interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_EVERY_N(::logger::ELogLevel::kInfo, name, interval, expression)
#define LOGGER_WARN_STREAM_EVERY_N_NAMED(name, interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_EVERY_N(::logger::ELogLevel::kWarn, name, interval, expression)
#define LOGGER_ERROR_STREAM_EVERY_N_NAMED(name, interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_EVERY_N(::logger::ELogLevel::kError, name, interval, expression)
#define LOGGER_FATAL_STREAM_EVERY_N_NAMED(name, interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_EVERY_N(::logger::ELogLevel::kFatal, name, interval, expression)

#define LOGGER_DEBUG_STREAM_THROTTLE_NAMED(name, interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_THROTTLE(::logger::ELogLevel::kDebug, name, interval, expression)
#define LOGGER_INFO_STREAM_THROTTLE_NAMED(name, interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_THROTTLE(::logger::ELogLevel::kInfo, name, interval, expression)
#define LOGGER_WARN_STREAM_THROTTLE_NAMED(name, interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_THROTTLE(::logger::ELogLevel::kWarn, name, interval, expression)
#define LOGGER_ERROR_STREAM_THROTTLE_NAMED(name, interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_THROTTLE(::logger::ELogLevel::kError, name, interval, expression)
#define LOGGER_FATAL_STREAM_THROTTLE_NAMED(name, interval, expression) \
    LOGGER_DETAIL_LOG_STREAM_NAMED_THROTTLE(::logger::ELogLevel::kFatal, name, interval, expression)

#define LOGGER_DETAIL_LOG_NAMED_ONCE(level, name, ...) \
    LOGGER_DETAIL_LOG_ONCE(level, name, __VA_ARGS__)
#define LOGGER_DETAIL_LOG_NAMED_EVERY_N(level, name, interval, ...) \
    LOGGER_DETAIL_LOG_EVERY_N(level, name, interval, __VA_ARGS__)
#define LOGGER_DETAIL_LOG_NAMED_THROTTLE(level, name, interval, ...) \
    LOGGER_DETAIL_LOG_THROTTLE(level, name, interval, __VA_ARGS__)

#define LOGGER_DEBUG_NAMED_ONCE(name, ...) \
    LOGGER_DETAIL_LOG_NAMED_ONCE(::logger::ELogLevel::kDebug, name, __VA_ARGS__)
#define LOGGER_INFO_NAMED_ONCE(name, ...) \
    LOGGER_DETAIL_LOG_NAMED_ONCE(::logger::ELogLevel::kInfo, name, __VA_ARGS__)
#define LOGGER_WARN_NAMED_ONCE(name, ...) \
    LOGGER_DETAIL_LOG_NAMED_ONCE(::logger::ELogLevel::kWarn, name, __VA_ARGS__)
#define LOGGER_ERROR_NAMED_ONCE(name, ...) \
    LOGGER_DETAIL_LOG_NAMED_ONCE(::logger::ELogLevel::kError, name, __VA_ARGS__)
#define LOGGER_FATAL_NAMED_ONCE(name, ...) \
    LOGGER_DETAIL_LOG_NAMED_ONCE(::logger::ELogLevel::kFatal, name, __VA_ARGS__)

#define LOGGER_DEBUG_NAMED_EVERY_N(name, interval, ...) \
    LOGGER_DETAIL_LOG_NAMED_EVERY_N(::logger::ELogLevel::kDebug, name, interval, __VA_ARGS__)
#define LOGGER_INFO_NAMED_EVERY_N(name, interval, ...) \
    LOGGER_DETAIL_LOG_NAMED_EVERY_N(::logger::ELogLevel::kInfo, name, interval, __VA_ARGS__)
#define LOGGER_WARN_NAMED_EVERY_N(name, interval, ...) \
    LOGGER_DETAIL_LOG_NAMED_EVERY_N(::logger::ELogLevel::kWarn, name, interval, __VA_ARGS__)
#define LOGGER_ERROR_NAMED_EVERY_N(name, interval, ...) \
    LOGGER_DETAIL_LOG_NAMED_EVERY_N(::logger::ELogLevel::kError, name, interval, __VA_ARGS__)
#define LOGGER_FATAL_NAMED_EVERY_N(name, interval, ...) \
    LOGGER_DETAIL_LOG_NAMED_EVERY_N(::logger::ELogLevel::kFatal, name, interval, __VA_ARGS__)

#define LOGGER_DEBUG_NAMED_THROTTLE(name, interval, ...) \
    LOGGER_DETAIL_LOG_NAMED_THROTTLE(::logger::ELogLevel::kDebug, name, interval, __VA_ARGS__)
#define LOGGER_INFO_NAMED_THROTTLE(name, interval, ...) \
    LOGGER_DETAIL_LOG_NAMED_THROTTLE(::logger::ELogLevel::kInfo, name, interval, __VA_ARGS__)
#define LOGGER_WARN_NAMED_THROTTLE(name, interval, ...) \
    LOGGER_DETAIL_LOG_NAMED_THROTTLE(::logger::ELogLevel::kWarn, name, interval, __VA_ARGS__)
#define LOGGER_ERROR_NAMED_THROTTLE(name, interval, ...) \
    LOGGER_DETAIL_LOG_NAMED_THROTTLE(::logger::ELogLevel::kError, name, interval, __VA_ARGS__)
#define LOGGER_FATAL_NAMED_THROTTLE(name, interval, ...) \
    LOGGER_DETAIL_LOG_NAMED_THROTTLE(::logger::ELogLevel::kFatal, name, interval, __VA_ARGS__)
