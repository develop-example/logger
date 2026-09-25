cmake_minimum_required(VERSION 3.16)

project(logger_example VERSION 0.4.0 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

option(LOGGER_BUILD_TESTS "Build logger tests" ON)

include(cmake/LoggerGenerator.cmake)

set(LOGGER_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
set(LOGGER_GENERATED_MODULE_HEADERS)

logger_module(
    MODULE MOTION
    MODULE_NAME Motion
    OUTPUT_FILE "${LOGGER_GENERATED_INCLUDE_DIR}/logger/logger_motion.h"
)
list(APPEND LOGGER_GENERATED_MODULE_HEADERS
    "${LOGGER_GENERATED_INCLUDE_DIR}/logger/logger_motion.h"
)

logger_module(
    MODULE VISION
    MODULE_NAME Vision
    OUTPUT_FILE "${LOGGER_GENERATED_INCLUDE_DIR}/logger/logger_vision.h"
)
list(APPEND LOGGER_GENERATED_MODULE_HEADERS
    "${LOGGER_GENERATED_INCLUDE_DIR}/logger/logger_vision.h"
)

add_library(logger STATIC
    src/config.cpp
    src/logger.cpp
)

add_library(logger::logger ALIAS logger)

target_compile_features(logger PUBLIC cxx_std_17)

target_include_directories(logger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${LOGGER_GENERATED_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(MSVC)
    target_compile_options(logger PRIVATE /W4)
else()
    target_compile_options(logger PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(logger_example
    example/main.cpp
)
target_link_libraries(logger_example PRIVATE logger::logger)

if(LOGGER_BUILD_TESTS AND BUILD_TESTING)
    add_executable(logger_test
        test/test_logger.cpp
    )
    target_link_libraries(logger_test PRIVATE logger::logger)
    add_test(NAME logger_test COMMAND logger_test)
endif()

install(TARGETS logger
    EXPORT loggerTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${LOGGER_GENERATED_MODULE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/logger
)
install(FILES config/logger.properties
    DESTINATION ${CMAKE_INSTALL_DATADIR}/logger
)

install(EXPORT loggerTargets
    FILE loggerTargets.cmake
    NAMESPACE logger::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/logger_example
)
