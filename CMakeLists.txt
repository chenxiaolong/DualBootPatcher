cmake_minimum_required(VERSION 3.1)

project(DualBootPatcher)
set(MBP_VERSION_MAJOR 8)
set(MBP_VERSION_MINOR 99)
set(MBP_VERSION_PATCH 13)
set(MBP_VERSION "${MBP_VERSION_MAJOR}.${MBP_VERSION_MINOR}.${MBP_VERSION_PATCH}")

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

# Allow version to be overridden in Jenkins
set(MBP_CI_VERSION "" CACHE STRING "Override version (continuous integration)")
if(MBP_CI_VERSION)
    if(NOT ${MBP_BUILD_TYPE} STREQUAL ci)
        message(FATAL_ERROR "Cannot override version number for non-CI builds")
    endif()
    set(MBP_VERSION ${MBP_CI_VERSION})
endif()

# Tests
set(MBP_ENABLE_TESTS TRUE CACHE BOOL "Enable building of tests")

if(MBP_ENABLE_TESTS)
    enable_testing()
endif()

# CPack versions
set(CPACK_PACKAGE_VERSION_MAJOR ${MBP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${MBP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${MBP_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${MBP_VERSION})

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

set(MBP_GLOBAL_INCLUDE_DIRS
    libmbcommon/include
    libmblog/include
    libmbpio/include
    libmbp/include
    libmbsparse/include
    libmbutil/include
)

include_directories(${MBP_GLOBAL_INCLUDE_DIRS})

# Main patcher
add_subdirectory(ndk)
add_subdirectory(libmbcommon)
add_subdirectory(libmblog)
add_subdirectory(libmbpio)
add_subdirectory(libmbp)
add_subdirectory(libmbsparse)
add_subdirectory(libmbutil)
add_subdirectory(data)
add_subdirectory(mbtool)
add_subdirectory(odinupdater)
add_subdirectory(libmiscstuff)
add_subdirectory(Android_GUI)
add_subdirectory(gui)
add_subdirectory(bootimgtool)
add_subdirectory(examples)
add_subdirectory(utilities)

include(CPack)
