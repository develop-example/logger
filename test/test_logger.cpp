#include <cassert>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <vector>

#include "logger/logger_motion.h"
#include "logger/logger.h"

namespace
{

class SlowStreamBuffer final : public std::streambuf
{
protected:
    std::streamsize xsputn(const char*, std::streamsize count) override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return count;
    }

    int overflow(int character) override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return character;
    }

    int sync() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return 0;
    }
};

class FailingStreamBuffer final : public std::streambuf
{
protected:
    std::streamsize xsputn(const char*, std::streamsize) override
    {
        throw std::runtime_error("intentional sink failure");
    }

    int overflow(int) override
    {
        throw std::runtime_error("intentional sink failure");
    }
};

}  // namespace

int main()
{
    assert(logger::version() != nullptr);
    assert(std::string(logger::version()) == "0.6.0");
    assert(logger::kVersionMajor == 0);
    assert(logger::kVersionMinor == 6);
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
    logger->flush();
    assert(output.str().empty());

    logger->print(logger::ELogLevel::kInfo, "Test", "test.cpp", 11, "main",
                  "value=%d", 7);
    logger->flush();
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
    logger->flush();
    assert(output.str().find("stream value=12") != std::string::npos);

    output.str("");
    output.clear();
    logger->setLogLevel(logger::ELogLevel::kDebug);
    LOGGER_DEBUG("debug %d", 1);
    LOGGER_INFO_NAMED("Named", "info %s", "message");
    LOGGER_WARN_STREAM_NAMED("NamedStream", "value=" << 9);
    MOTION_LOG_INFO("module %d", 2);
    MOTION_LOG_ERROR_STREAM("error code=" << 17);
    logger->flush();
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
    logger->flush();
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
    logger->flush();
    assert(output.str().empty());
    logger->print(logger::ELogLevel::kFatal, "Test", "test.cpp", 14, "main", "visible");
    logger->flush();
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
    logger->flush();
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

    const std::filesystem::path sink_directory = "stage5-output";
    const std::filesystem::path sink_path = sink_directory / "logger.log";
    const std::string sink_config_path = "stage5-sinks.properties";
    {
        std::ofstream config(sink_config_path);
        assert(config);
        config << "logger.level=DEBUG\n"
               << "logger.sinks=file\n"
               << "logger.file.path=" << sink_path.string() << "\n"
               << "logger.file.append=false\n";
    }
    std::filesystem::remove_all(sink_directory);
    assert(logger->loadConfig(sink_config_path));
    logger->resetOutput();
    LOGGER_INFO("file sink message");
    logger->flush();
    assert(std::filesystem::exists(sink_path));
    std::ifstream first_file(sink_path);
    const std::string first_contents((std::istreambuf_iterator<char>(first_file)),
                                     std::istreambuf_iterator<char>());
    assert(first_contents.find("file sink message") != std::string::npos);

    {
        std::ofstream config(sink_config_path);
        assert(config);
        config << "logger.level=DEBUG\n"
               << "logger.sinks=console,file\n"
               << "logger.file.path=" << sink_path.string() << "\n"
               << "logger.file.append=true\n";
    }
    assert(logger->reloadConfig());
    LOGGER_WARN("appended sink message");
    logger->flush();
    std::ifstream second_file(sink_path);
    const std::string second_contents((std::istreambuf_iterator<char>(second_file)),
                                      std::istreambuf_iterator<char>());
    assert(second_contents.size() > first_contents.size());
    assert(second_contents.find("appended sink message") != std::string::npos);

    {
        std::ofstream invalid(sink_config_path);
        assert(invalid);
        invalid << "logger.level=DEBUG\n"
                << "logger.sinks=file\n"
                << "logger.file.path=/dev/null/logger.log\n";
    }
    assert(!logger->reloadConfig());
    assert(logger->getConfigPath() == sink_config_path);
    LOGGER_ERROR("old sink remains active");
    logger->flush();
    std::ifstream preserved_file(sink_path);
    const std::string preserved_contents((std::istreambuf_iterator<char>(preserved_file)),
                                         std::istreambuf_iterator<char>());
    assert(preserved_contents.find("old sink remains active") != std::string::npos);

    FailingStreamBuffer failing_buffer;
    std::ostream failing_output(&failing_buffer);
    failing_output.exceptions(std::ios::badbit);
    logger->setOutput(failing_output);
    const logger::QueueStats before_sink_error = logger->getQueueStats();
    LOGGER_ERROR("sink failure must not stop worker");
    logger->flush();
    const logger::QueueStats after_sink_error = logger->getQueueStats();
    assert(after_sink_error.sink_errors > before_sink_error.sink_errors);
    logger->resetOutput();

    std::filesystem::remove(sink_config_path);
    std::filesystem::remove_all(sink_directory);

    logger->setLogLevel(logger::ELogLevel::kDebug);
    logger->setQueueOverflowPolicy(logger::EQueueOverflowPolicy::kDropNewest);
    assert(!logger->setQueueCapacity(0));
    assert(logger->setQueueCapacity(1));

    SlowStreamBuffer slow_buffer;
    std::ostream slow_output(&slow_buffer);
    logger->setOutput(slow_output);
    const logger::QueueStats before_queue_test = logger->getQueueStats();
    for (int message = 0; message < 100; ++message)
    {
        LOGGER_INFO("queue message=%d", message);
    }
    logger->flush();
    const logger::QueueStats after_queue_test = logger->getQueueStats();
    assert(after_queue_test.capacity == 1);
    assert(after_queue_test.size == 0);
    assert(after_queue_test.peak_size >= before_queue_test.peak_size);
    assert(after_queue_test.accepted >= before_queue_test.accepted);
    assert(after_queue_test.dropped > before_queue_test.dropped);
    assert(after_queue_test.dropped_info > before_queue_test.dropped_info);

    logger->setQueueOverflowPolicy(logger::EQueueOverflowPolicy::kDropLowPriority);
    logger->setQueueCapacity(8);
    logger->setOutput(output);
    logger->flush();
    const logger::QueueStats before_shutdown = logger->getQueueStats();
    logger->shutdown();
    logger->shutdown();
    LOGGER_INFO("after shutdown");
    logger->flush();
    const logger::QueueStats after_shutdown = logger->getQueueStats();
    assert(after_shutdown.dropped > before_shutdown.dropped);
    assert(!logger->setQueueCapacity(16));

    logger->resetOutput();
    return 0;
}
