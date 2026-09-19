#pragma once

#include <cstdint>
#include <chrono>
#include <sstream>
#include <string>
#include <thread>

#include <iosfwd>

namespace logger
{

inline constexpr int kVersionMajor = 0;
inline constexpr int kVersionMinor = 2;
inline constexpr int kVersionPatch = 0;

enum class ELogLevel : std::uint8_t
{
    kDebug = 0,
    kInfo,
    kWarn,
    kError,
    kFatal,
};

const char* toString(ELogLevel level) noexcept;

struct LogRecord
{
    std::chrono::system_clock::time_point timestamp;
    ELogLevel level{ELogLevel::kInfo};
    std::string name;
    std::string file;
    std::uint32_t line{0};
    std::string function;
    std::thread::id thread_id;
    std::string message;
};

class ILogger
{
public:
    static ILogger* getInstance() noexcept;

    virtual ~ILogger() = default;

    virtual void print(ELogLevel level, const char* name, const char* file,
                       std::uint32_t line, const char* function,
                       const char* format, ...) = 0;

    virtual void print(ELogLevel level, const char* name, const char* file,
                       std::uint32_t line, const char* function,
                       const std::stringstream& stream) = 0;

    virtual void setLogLevel(ELogLevel level) noexcept = 0;
    virtual ELogLevel getLogLevel() const noexcept = 0;
    virtual bool shouldLog(ELogLevel level) const noexcept = 0;

    // The default destination is std::clog. The caller must keep the stream
    // alive until another output stream is selected or the process exits.
    virtual void setOutput(std::ostream& output) noexcept = 0;
    virtual void resetOutput() noexcept = 0;
    virtual void flush() noexcept = 0;
};

// Stage 1 provides a synchronous, dependency-free logger. Macros and
// asynchronous backends are added in later stages.
const char* version() noexcept;

}  // namespace logger
