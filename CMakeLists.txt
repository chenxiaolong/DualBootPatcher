cmake_minimum_required(VERSION 3.1)

project(DualBootPatcher)
set(MBP_VERSION_MAJOR 8)
set(MBP_VERSION_MINOR 99)
set(MBP_VERSION_PATCH 12)
set(MBP_VERSION "${MBP_VERSION_MAJOR}.${MBP_VERSION_MINOR}.${MBP_VERSION_PATCH}")

# Allow version to be overridden in Jenkins
set(MBP_CI_VERSION "" CACHE STRING "Override version (continuous integration)")
if(MBP_CI_VERSION)
    set(MBP_VERSION ${MBP_CI_VERSION})
endif()

set(CPACK_PACKAGE_VERSION_MAJOR ${MBP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${MBP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${MBP_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${MBP_VERSION})

# Build target and type
set(MBP_BUILD_TARGET "desktop"
    CACHE STRING "Target platform (desktop or android)")
set(MBP_BUILD_TYPE "debug"
    CACHE STRING "Target build type (release, debug, or ci)")

# Verify build target and type
if(NOT ${MBP_BUILD_TARGET} STREQUAL desktop
        AND NOT ${MBP_BUILD_TARGET} STREQUAL android)
    message(FATAL_ERROR "Invalid build target: ${MBP_BUILD_TARGET}")
endif()
if(NOT ${MBP_BUILD_TYPE} STREQUAL release
        AND NOT ${MBP_BUILD_TYPE} STREQUAL debug
        AND NOT ${MBP_BUILD_TYPE} STREQUAL ci)
    message(FATAL_ERROR "Invalid build type: ${MBP_BUILD_TYPE}")
endif()

# Code signing options
include(cmake/ConfigSigning.cmake)

if(${MBP_BUILD_TARGET} STREQUAL android)
    include(cmake/ConfigAndroid.cmake)
elseif(${MBP_BUILD_TARGET} STREQUAL desktop)
    include(cmake/ConfigDesktop.cmake)
endif()

include(cmake/LogicalCoreCount.cmake)
include(cmake/AndroidNDK.cmake)

# Third party binaries
add_subdirectory(thirdparty)

# Main patcher
add_subdirectory(libmbpio)
add_subdirectory(libmbp)
add_subdirectory(data)
add_subdirectory(mbtool)
add_subdirectory(libmiscstuff)
add_subdirectory(Android_GUI)
add_subdirectory(gui)
add_subdirectory(bootimgtool)
add_subdirectory(utilities)

include(CPack)
