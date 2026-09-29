#pragma once

#include "logger/logger_api.h"

#include <initializer_list>
#include <optional>
#include <string>

namespace logger
{

class LogContext final
{
public:
    static bool set(std::string key, std::string value);
    static std::optional<std::string> get(const std::string& key);
    static bool erase(const std::string& key) noexcept;
    static void clear() noexcept;

    static LogContextSnapshot snapshot();
    static bool restore(const LogContextSnapshot& snapshot);
};

class ScopedLogContext final
{
public:
    explicit ScopedLogContext(std::initializer_list<LogField> fields);
    explicit ScopedLogContext(const LogContextSnapshot& fields);
    ~ScopedLogContext() noexcept;

    ScopedLogContext(const ScopedLogContext&) = delete;
    ScopedLogContext& operator=(const ScopedLogContext&) = delete;
    ScopedLogContext(ScopedLogContext&&) = delete;
    ScopedLogContext& operator=(ScopedLogContext&&) = delete;

private:
    LogContextSnapshot previous_;
};

}  // namespace logger
