#include "logger/log_context.h"

#include <stdexcept>

namespace logger
{

namespace
{

thread_local LogContextSnapshot current_context;

bool hasEmptyKey(const LogContextSnapshot& fields) noexcept
{
    for (const auto& field : fields)
    {
        if (field.key.empty())
        {
            return true;
        }
    }
    return false;
}

void upsert(LogContextSnapshot& fields, const LogField& field)
{
    for (auto& existing : fields)
    {
        if (existing.key == field.key)
        {
            existing.value = field.value;
            return;
        }
    }
    fields.push_back(field);
}

LogContextSnapshot mergedWith(const LogContextSnapshot& base, const LogContextSnapshot& overlay)
{
    LogContextSnapshot merged = base;
    for (const auto& field : overlay)
    {
        upsert(merged, field);
    }
    return merged;
}

}  // namespace

bool LogContext::set(std::string key, std::string value)
{
    if (key.empty())
    {
        return false;
    }
    upsert(current_context, LogField{std::move(key), std::move(value)});
    return true;
}

std::optional<std::string> LogContext::get(const std::string& key)
{
    if (key.empty())
    {
        return std::nullopt;
    }
    for (const auto& field : current_context)
    {
        if (field.key == key)
        {
            return field.value;
        }
    }
    return std::nullopt;
}

bool LogContext::erase(const std::string& key) noexcept
{
    if (key.empty())
    {
        return false;
    }
    for (auto iterator = current_context.begin(); iterator != current_context.end(); ++iterator)
    {
        if (iterator->key == key)
        {
            current_context.erase(iterator);
            return true;
        }
    }
    return false;
}

void LogContext::clear() noexcept
{
    current_context.clear();
}

LogContextSnapshot LogContext::snapshot()
{
    return current_context;
}

bool LogContext::restore(const LogContextSnapshot& snapshot)
{
    if (hasEmptyKey(snapshot))
    {
        return false;
    }

    LogContextSnapshot normalized;
    normalized.reserve(snapshot.size());
    for (const auto& field : snapshot)
    {
        upsert(normalized, field);
    }
    current_context = std::move(normalized);
    return true;
}

ScopedLogContext::ScopedLogContext(std::initializer_list<LogField> fields)
    : ScopedLogContext(LogContextSnapshot(fields))
{
}

ScopedLogContext::ScopedLogContext(const LogContextSnapshot& fields)
    : previous_(LogContext::snapshot())
{
    if (hasEmptyKey(fields))
    {
        throw std::invalid_argument("log context key must not be empty");
    }

    const LogContextSnapshot merged = mergedWith(previous_, fields);
    if (!LogContext::restore(merged))
    {
        throw std::invalid_argument("invalid log context");
    }
}

ScopedLogContext::~ScopedLogContext() noexcept
{
    try
    {
        if (!LogContext::restore(previous_))
        {
            LogContext::clear();
        }
    }
    catch (...)
    {
        LogContext::clear();
    }
}

}  // namespace logger
