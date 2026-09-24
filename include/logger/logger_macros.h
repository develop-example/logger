#pragma once

#include "logger/logger_api.h"

#if defined(_MSC_VER)
#define LOGGER_DETAIL_FUNCTION __FUNCSIG__
#else
#define LOGGER_DETAIL_FUNCTION __PRETTY_FUNCTION__
#endif

#define LOGGER_DETAIL_LOG(level, name, ...)                                                   \
    do                                                                                         \
    {                                                                                          \
        auto* logger_detail_instance = ::logger::ILogger::getInstance();                       \
        if (logger_detail_instance->shouldLog(level))                                          \
        {                                                                                      \
            logger_detail_instance->print(level, name, __FILE__, __LINE__,                     \
                                          LOGGER_DETAIL_FUNCTION, __VA_ARGS__);                 \
        }                                                                                      \
    } while (false)

#define LOGGER_DETAIL_LOG_STREAM(level, name, expression)                                      \
    do                                                                                         \
    {                                                                                          \
        auto* logger_detail_instance = ::logger::ILogger::getInstance();                       \
        if (logger_detail_instance->shouldLog(level))                                          \
        {                                                                                      \
            std::stringstream logger_detail_stream;                                            \
            logger_detail_stream << expression;                                                \
            logger_detail_instance->print(level, name, __FILE__, __LINE__,                     \
                                          LOGGER_DETAIL_FUNCTION, logger_detail_stream);        \
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
