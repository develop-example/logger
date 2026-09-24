#include "logger/logger_motion.h"
#include "logger/logger_vision.h"
#include "logger/logger.h"

int main()
{
    auto* log = logger::ILogger::getInstance();
    log->setLogLevel(logger::ELogLevel::kDebug);

    LOGGER_DEBUG("logger example, version %s", logger::version());
    LOGGER_INFO("general logger macro");
    LOGGER_WARN_NAMED("Example", "named logger macro, value=%d", 42);
    LOGGER_INFO_STREAM("stream value=" << 3.14);
    MOTION_LOG_INFO("motion module started");
    MOTION_LOG_WARN_STREAM("target speed=" << 0.5);
    VISION_LOG_ERROR("camera initialization failed: %s", "not connected");

    log->setLogLevel(logger::ELogLevel::kWarn);
    LOGGER_INFO("this message is filtered");
    LOGGER_ERROR("error messages are still visible");
    log->flush();
    return 0;
}
