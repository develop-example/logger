#include <iostream>

#include "logger/logger_motion.h"
#include "logger/logger_vision.h"
#include "logger/logger.h"
#include "logger/log_context.h"

int main()
{
    auto* log = logger::ILogger::getInstance();
    if (!log->loadConfig("config/logger.properties"))
    {
        return 1;
    }

    LOGGER_DEBUG("logger example, version %s", logger::version());
    LOGGER_INFO("general logger macro");
    logger::ScopedLogContext request_context{{"request_id", "example-1001"},
                                             {"component", "demo"}};
    LOGGER_INFO("context-aware logger macro");
    LOGGER_WARN_NAMED("Example", "named logger macro, value=%d", 42);
    LOGGER_INFO_STREAM("stream value=" << 3.14);
    MOTION_LOG_INFO("motion module started");
    MOTION_LOG_WARN_STREAM("target speed=" << 0.5);
    VISION_LOG_ERROR("camera initialization failed: %s", "not connected");

    log->setLogLevel("Motion", logger::ELogLevel::kError);
    MOTION_LOG_INFO("this module message is filtered after runtime override");
    MOTION_LOG_ERROR("this module error remains visible");
    log->setLogLevel(logger::ELogLevel::kWarn);
    LOGGER_INFO("this message is filtered");
    LOGGER_ERROR("error messages are still visible");
    log->flush();

    const logger::QueueStats stats = log->getQueueStats();
    std::cout << "accepted=" << stats.accepted << " dropped=" << stats.dropped
              << " peak_size=" << stats.peak_size << '\n';
    log->shutdown();
    return 0;
}
