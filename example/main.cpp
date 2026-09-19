#include <iostream>

#include "logger/logger.h"

int main()
{
    std::cout << "logger example, version " << logger::version() << '\n';
    return 0;
}
