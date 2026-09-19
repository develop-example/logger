#include <cassert>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "logger/logger.h"

int main()
{
    assert(logger::version() != nullptr);
    assert(std::string(logger::version()) == "0.2.0");
    assert(logger::kVersionMajor == 0);
    assert(logger::kVersionMinor == 2);
    assert(logger::kVersionPatch == 0);

    auto* logger = logger::ILogger::getInstance();
    std::ostringstream output;
    logger->setOutput(output);
    logger->setLogLevel(logger::ELogLevel::kInfo);

    assert(!logger->shouldLog(logger::ELogLevel::kDebug));
    assert(logger->shouldLog(logger::ELogLevel::kInfo));
    assert(logger->shouldLog(logger::ELogLevel::kError));

    logger->print(logger::ELogLevel::kDebug, "Test", "test.cpp", 10, "main",
                  "filtered value=%d", 7);
    assert(output.str().empty());

    logger->print(logger::ELogLevel::kInfo, "Test", "test.cpp", 11, "main",
                  "value=%d", 7);
    const std::string formatted = output.str();
    assert(formatted.find("[INFO]") != std::string::npos);
    assert(formatted.find("[Test]") != std::string::npos);
    assert(formatted.find("[test.cpp:11]") != std::string::npos);
    assert(formatted.find("value=7") != std::string::npos);

    output.str("");
    output.clear();
    std::stringstream stream;
    stream << "stream value=" << 12;
    logger->print(logger::ELogLevel::kWarn, "Test", "test.cpp", 12, "main", stream);
    assert(output.str().find("stream value=12") != std::string::npos);

    logger->setLogLevel(logger::ELogLevel::kFatal);
    output.str("");
    output.clear();
    logger->print(logger::ELogLevel::kError, "Test", "test.cpp", 13, "main", "hidden");
    assert(output.str().empty());
    logger->print(logger::ELogLevel::kFatal, "Test", "test.cpp", 14, "main", "visible");
    assert(output.str().find("visible") != std::string::npos);

    logger->setLogLevel(logger::ELogLevel::kInfo);
    output.str("");
    output.clear();
    constexpr int kThreadCount = 4;
    constexpr int kMessagesPerThread = 25;
    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (int thread = 0; thread < kThreadCount; ++thread)
    {
        workers.emplace_back([logger, thread] {
            for (int message = 0; message < kMessagesPerThread; ++message)
            {
                logger->print(logger::ELogLevel::kInfo, "Worker", "thread.cpp", 20, "worker",
                              "worker=%d message=%d", thread, message);
            }
        });
    }
    for (auto& worker : workers)
    {
        worker.join();
    }
    const std::string threaded = output.str();
    std::size_t position = 0;
    int line_count = 0;
    while ((position = threaded.find(" worker=", position)) != std::string::npos)
    {
        ++line_count;
        ++position;
    }
    assert(line_count == kThreadCount * kMessagesPerThread);

    logger->resetOutput();
    logger->setLogLevel(logger::ELogLevel::kDebug);
    return 0;
}
