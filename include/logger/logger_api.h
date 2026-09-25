#pragma once

#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <sstream>
#include <string>
#include <thread>

namespace logger
{

inline constexpr int kVersionMajor = 0;
inline constexpr int kVersionMinor = 4;
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
    virtual void setLogLevel(const std::string& name, ELogLevel level) noexcept = 0;
    virtual ELogLevel getLogLevel() const noexcept = 0;
    virtual ELogLevel getLogLevel(const std::string& name) const noexcept = 0;
    virtual bool shouldLog(ELogLevel level) const noexcept = 0;
    virtual bool shouldLog(const char* name, ELogLevel level) const noexcept = 0;

    // A failed load leaves the currently active configuration unchanged.
    virtual bool loadConfig(const std::string& path) = 0;
    virtual bool reloadConfig() = 0;
    virtual std::string getConfigPath() const = 0;

    // The caller must keep a custom stream alive while it is selected.
    virtual void setOutput(std::ostream& output) noexcept = 0;
    virtual void resetOutput() noexcept = 0;
    virtual void flush() noexcept = 0;
};

const char* version() noexcept;

}  // namespace logger
