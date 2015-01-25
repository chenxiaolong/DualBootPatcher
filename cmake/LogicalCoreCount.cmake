# Modified version of the original script from David Cole:
# http://www.kitware.com/blog/home/post/63

if(LogicalCoreCount_included)
    return()
endif()
set(LogicalCoreCount_included TRUE)

if(NOT DEFINED PROCESSOR_COUNT)
    # Unknown:
    set(PROCESSOR_COUNT 1)

    # Linux:
    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(cpuinfo_file "/proc/cpuinfo")
        if(EXISTS "${cpuinfo_file}")
            file(
                STRINGS "${cpuinfo_file}" procs
                REGEX "^core id"
            )
            list(REMOVE_DUPLICATES procs)
            list(LENGTH procs PROCESSOR_COUNT)
        endif()
    endif()

    # Mac:
    if(APPLE)
        find_program(cmd_sysctl "sysctl")
        if(cmd_sysctl)
            execute_process(
                COMMAND ${cmd_sysctl} -n hw.ncpu
                OUTPUT_VARIABLE PROCESSOR_COUNT
            )
        endif()
    endif()

    # Windows:
    if(WIN32)
        set(PROCESSOR_COUNT "$ENV{NUMBER_OF_PROCESSORS}")
    endif()

    if("${PROCESSOR_COUNT}" STREQUAL "")
        set(PROCESSOR_COUNT 4)
    endif()

    message(STATUS "Processor count: ${PROCESSOR_COUNT}")
endif()
