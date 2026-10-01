#pragma once

#include <memory>
#include <string>
#include <vector>

#include "config.h"

namespace logger::detail
{

class ILogSink
{
public:
    virtual ~ILogSink() = default;
    virtual void write(const std::string& line) = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
};

bool createSinks(const SinkConfig& config, std::vector<std::unique_ptr<ILogSink>>& sinks,
                 std::string& error);
std::unique_ptr<ILogSink> createStreamSink(std::ostream& output);

}  // namespace logger::detail
