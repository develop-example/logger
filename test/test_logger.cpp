#include <cassert>
#include <string>

#include "logger/logger.h"

int main()
{
    assert(logger::version() != nullptr);
    assert(std::string(logger::version()) == "0.1.0");
    assert(logger::kVersionMajor == 0);
    assert(logger::kVersionMinor == 1);
    assert(logger::kVersionPatch == 0);
    return 0;
}
