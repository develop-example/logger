#include <iostream>
#include <sstream>

#include "logger/logger.h"

int main()
{
    auto* logger = logger::ILogger::getInstance();

    std::cout << "logger example, version " << logger::version() << '\n';

    logger->setLogLevel(logger::ELogLevel::kDebug);
    logger->print(logger::ELogLevel::kDebug, "Example", __FILE__, __LINE__, __func__,
                  "debug value=%d", 42);
    logger->print(logger::ELogLevel::kInfo, "Example", __FILE__, __LINE__, __func__,
                  "an informational message");

    std::stringstream stream;
    stream << "stream value=" << 3.14;
    logger->print(logger::ELogLevel::kWarn, "Example", __FILE__, __LINE__, __func__, stream);

    logger->setLogLevel(logger::ELogLevel::kWarn);
    logger->print(logger::ELogLevel::kInfo, "Example", __FILE__, __LINE__, __func__,
                  "this message is filtered");
    logger->print(logger::ELogLevel::kError, "Example", __FILE__, __LINE__, __func__,
                  "error messages are still visible");
    logger->flush();
    return 0;
}
