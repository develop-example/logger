#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "logger/logger_motion.h"
#include "logger/logger.h"

int main()
{
    assert(logger::version() != nullptr);
    assert(std::string(logger::version()) == "0.4.0");
    assert(logger::kVersionMajor == 0);
    assert(logger::kVersionMinor == 4);
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

    output.str("");
    output.clear();
    logger->setLogLevel(logger::ELogLevel::kDebug);
    LOGGER_DEBUG("debug %d", 1);
    LOGGER_INFO_NAMED("Named", "info %s", "message");
    LOGGER_WARN_STREAM_NAMED("NamedStream", "value=" << 9);
    MOTION_LOG_INFO("module %d", 2);
    MOTION_LOG_ERROR_STREAM("error code=" << 17);
    const std::string macro_output = output.str();
    assert(macro_output.find("[DEBUG]") != std::string::npos);
    assert(macro_output.find("[Named]") != std::string::npos);
    assert(macro_output.find("[NamedStream]") != std::string::npos);
    assert(macro_output.find("[Motion]") != std::string::npos);
    assert(macro_output.find("module 2") != std::string::npos);
    assert(macro_output.find("error code=17") != std::string::npos);
    assert(macro_output.find("test_logger.cpp:") != std::string::npos);
    assert(macro_output.find("main()") != std::string::npos);

    int stream_evaluations = 0;
    logger->setLogLevel(logger::ELogLevel::kInfo);
    LOGGER_DEBUG_STREAM(++stream_evaluations);
    MOTION_LOG_DEBUG_STREAM(++stream_evaluations);
    assert(stream_evaluations == 0);
    LOGGER_INFO_STREAM(++stream_evaluations);
    assert(stream_evaluations == 1);

    bool else_branch = false;
    if (false)
        LOGGER_INFO("unreachable");
    else
        else_branch = true;
    assert(else_branch);

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

    const std::string config_path = "stage3_test_logger.properties";
    {
        std::ofstream config(config_path);
        assert(config);
        config << "# comments and whitespace are supported\n"
               << "logger.level = WARN\n"
               << "logger.Motion.level = debug\n"
               << "logger.Motion.Controller.level=ERROR\n"
               << "logger.Vision.level=FATAL\n";
    }

    assert(logger->loadConfig(config_path));
    assert(logger->getConfigPath() == config_path);
    assert(logger->getLogLevel() == logger::ELogLevel::kWarn);
    assert(logger->getLogLevel("Motion") == logger::ELogLevel::kDebug);
    assert(logger->getLogLevel("Motion.Controller.Sensor") == logger::ELogLevel::kError);
    assert(logger->getLogLevel("Motion.Other") == logger::ELogLevel::kDebug);
    assert(logger->getLogLevel("Unknown") == logger::ELogLevel::kWarn);
    assert(logger->shouldLog("Motion", logger::ELogLevel::kDebug));
    assert(!logger->shouldLog("Vision", logger::ELogLevel::kError));
    assert(logger->shouldLog("Vision", logger::ELogLevel::kFatal));

    logger->setLogLevel("Motion", logger::ELogLevel::kFatal);
    assert(logger->getLogLevel("Motion") == logger::ELogLevel::kFatal);
    logger->setLogLevel("Motion", logger::ELogLevel::kDebug);

    {
        std::ofstream invalid(config_path);
        assert(invalid);
        invalid << "logger.level=not-a-level\n";
    }
    assert(!logger->reloadConfig());
    assert(logger->getLogLevel() == logger::ELogLevel::kWarn);
    assert(logger->getLogLevel("Motion") == logger::ELogLevel::kDebug);

    {
        std::ofstream invalid(config_path);
        assert(invalid);
        invalid << "logger.unknown.property=INFO\n";
    }
    assert(!logger->reloadConfig());
    assert(logger->getLogLevel() == logger::ELogLevel::kWarn);

    std::remove(config_path.c_str());
    assert(!logger->loadConfig("missing-stage3-config.properties"));
    assert(logger->getConfigPath() == config_path);
    assert(logger->getLogLevel() == logger::ELogLevel::kWarn);

    logger->resetOutput();
    logger->setLogLevel(logger::ELogLevel::kDebug);
    return 0;
}
