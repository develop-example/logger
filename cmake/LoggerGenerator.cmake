include_guard(GLOBAL)

set(LOGGER_GENERATOR_TEMPLATE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(logger_module)
    cmake_parse_arguments(MODULE "" "MODULE;MODULE_NAME;OUTPUT_FILE" "" ${ARGN})

    if(NOT MODULE_MODULE)
        message(FATAL_ERROR "logger_module: MODULE is required")
    endif()
    if(NOT MODULE_MODULE_NAME)
        message(FATAL_ERROR "logger_module: MODULE_NAME is required")
    endif()
    if(NOT MODULE_OUTPUT_FILE)
        message(FATAL_ERROR "logger_module: OUTPUT_FILE is required")
    endif()

    get_filename_component(output_directory "${MODULE_OUTPUT_FILE}" DIRECTORY)
    file(MAKE_DIRECTORY "${output_directory}")

    set(module_prefix "${MODULE_MODULE}")
    set(module_name "${MODULE_MODULE_NAME}")
    configure_file(
        "${LOGGER_GENERATOR_TEMPLATE_DIR}/LoggerModule.h.in"
        "${MODULE_OUTPUT_FILE}"
        @ONLY
    )
endfunction()
