#include "logger/logger_motion.h"
#include "logger/logger_vision.h"
#include "logger/logger.h"

int main()
{
    auto* log = logger::ILogger::getInstance();
    if (!log->loadConfig("config/logger.properties"))
    {
        return 1;
    }

    LOGGER_DEBUG("logger example, version %s", logger::version());
    LOGGER_INFO("general logger macro");
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
    return 0;
}
