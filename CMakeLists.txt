cmake_minimum_required(VERSION 3.0)

project(DualBootPatcher)
set(MBP_VERSION_MAJOR 9)
set(MBP_VERSION_MINOR 0)
set(MBP_VERSION_PATCH 0Alpha6)
set(MBP_VERSION "${MBP_VERSION_MAJOR}.${MBP_VERSION_MINOR}.${MBP_VERSION_PATCH}")

option(BUILD_ANDROID "Build for Android" OFF)

if(BUILD_ANDROID)
    include(cmake/ConfigAndroid.cmake)
else()
    include(cmake/ConfigDesktop.cmake)
endif()

include (cmake/AndroidNDK.cmake)

# Main patcher
add_subdirectory(libmbp)
add_subdirectory(data)
add_subdirectory(mbtool)
add_subdirectory(syncdaemon)

if(BUILD_ANDROID)
    add_subdirectory(libmountpoint)
    add_subdirectory(libloki)
else()
    add_subdirectory(shortcuts)
    add_subdirectory(gui)
endif()

# Third party binaries
add_subdirectory(thirdparty)

include(CPack)
