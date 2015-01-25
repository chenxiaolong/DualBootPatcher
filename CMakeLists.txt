cmake_minimum_required(VERSION 3.1)

project(DualBootPatcher)
set(MBP_VERSION_MAJOR 9)
set(MBP_VERSION_MINOR 0)
set(MBP_VERSION_PATCH 0Alpha6)
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

option(MBP_BUILD_ANDROID "Build for Android" OFF)

if(MBP_BUILD_ANDROID)
    include(cmake/ConfigAndroid.cmake)
else()
    include(cmake/ConfigDesktop.cmake)
endif()

include(cmake/AndroidNDK.cmake)
include(cmake/LogicalCoreCount.cmake)

# Main patcher
add_subdirectory(libmbp)
add_subdirectory(data)
add_subdirectory(mbtool)

if(MBP_BUILD_ANDROID)
    add_subdirectory(libmiscstuff)
    add_subdirectory(Android_GUI)
else()
    add_subdirectory(gui)
endif()

# Third party binaries
add_subdirectory(thirdparty)

include(CPack)
