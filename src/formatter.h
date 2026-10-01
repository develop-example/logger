#pragma once

#include <memory>
#include <string>

#include "config.h"

namespace logger::detail
{

class ILogFormatter
{
public:
    virtual ~ILogFormatter() = default;
    virtual std::string format(const LogRecord& record) const = 0;
};

std::unique_ptr<ILogFormatter> createFormatter(const FormatterConfig& config);

}  // namespace logger::detail
