cmake_minimum_required(VERSION 3.6)

# Add our custom module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Add custom module path for AndroidCompat platform on CMake 3.7
if(NOT CMAKE_VERSION VERSION_LESS 3.7)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/cmake-3.7")
endif()

project(DualBootPatcher CXX C ASM)
set(MBP_VERSION_MAJOR 9)
set(MBP_VERSION_MINOR 1)
set(MBP_VERSION_PATCH 0)
set(MBP_VERSION "${MBP_VERSION_MAJOR}.${MBP_VERSION_MINOR}.${MBP_VERSION_PATCH}")

# Build target and type
set(MBP_BUILD_TARGET "desktop"
    CACHE STRING "Target platform (desktop or android)")
set(MBP_BUILD_TYPE "debug"
    CACHE STRING "Target build type (release, debug, or ci)")
set(MBP_SYSTEM_BUILD_TYPE "release"
    CACHE STRING "Target build type for Android system components (release or debug)")

# Build types:
# - desktop: Standard build for PCs
# - android: Standard build for Android
# - android-app: Build libraries to be used for the Android app
#                (not to be built manually)
# - android-system: Build executables to be included in the distrbution
#                   (not to be built manually)
# - hosttools: Builds tools needed on the host when cross-compiling
#              (not to be built manually)

# Verify build target and type
if(NOT ${MBP_BUILD_TARGET} STREQUAL desktop
        AND NOT ${MBP_BUILD_TARGET} STREQUAL android
        AND NOT ${MBP_BUILD_TARGET} STREQUAL android-app
        AND NOT ${MBP_BUILD_TARGET} STREQUAL android-system
        AND NOT ${MBP_BUILD_TARGET} STREQUAL hosttools)
    message(FATAL_ERROR "Invalid build target: ${MBP_BUILD_TARGET}")
endif()
if(NOT ${MBP_BUILD_TYPE} STREQUAL release
        AND NOT ${MBP_BUILD_TYPE} STREQUAL debug
        AND NOT ${MBP_BUILD_TYPE} STREQUAL ci)
    message(FATAL_ERROR "Invalid build type: ${MBP_BUILD_TYPE}")
endif()
if(NOT ${MBP_SYSTEM_BUILD_TYPE} STREQUAL release
        AND NOT ${MBP_SYSTEM_BUILD_TYPE} STREQUAL debug)
    message(FATAL_ERROR "Invalid system build type: ${MBP_SYSTEM_BUILD_TYPE}")
endif()

# Require at least Android NDK r11
if(${MBP_BUILD_TARGET} STREQUAL android-app
        OR ${MBP_BUILD_TARGET} STREQUAL android-system)
    # r13 bug: ANDROID_NDK_RELEASE has newline at the end
    string(REGEX REPLACE "^r(.+)[\r\n]+$" "\\1" ndk_release "${ANDROID_NDK_RELEASE}")
    set(ndk_min_version 130.0)
    set(ndk_max_version 132.0)

    # FIXME: Due to https://github.com/android-ndk/ndk/issues/222, we need a
    # custom toolchain file. Since we can't guarantee that the custom toolchain
    # file will work with newer versions of the NDK, we'll depend on an exact
    # version.
    set(ndk_version_whitelist
        130.3315539
        131.3345770
    )

    if(DEFINED ndk_version_whitelist
            AND NOT ndk_release IN_LIST ndk_version_whitelist)
        message(FATAL_ERROR
                "Requires NDK version in list: ${ndk_version_whitelist}"
                " (detected: ${ndk_release})")
    elseif(ndk_release VERSION_LESS ndk_min_version
            OR NOT ndk_release VERSION_LESS ndk_max_version)
        message(FATAL_ERROR
                "Requires ${ndk_min_version} <= NDK version < ${ndk_max_version}"
                " (detected: ${ndk_release})")
    endif()
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

# Ensure CMAKE_BUILD_TYPE is set
if(NOT CMAKE_BUILD_TYPE)
    if(${MBP_BUILD_TYPE} STREQUAL release)
        set(NEW_CMAKE_BUILD_TYPE Release)
    elseif(${MBP_BUILD_TYPE} STREQUAL debug)
        set(NEW_CMAKE_BUILD_TYPE Debug)
    elseif(${MBP_BUILD_TYPE} STREQUAL ci)
        set(NEW_CMAKE_BUILD_TYPE Release)
    endif()

    message(STATUS "Setting CMAKE_BUILD_TYPE to ${NEW_CMAKE_BUILD_TYPE} because it wasn't explicitly specified")
    set(CMAKE_BUILD_TYPE ${NEW_CMAKE_BUILD_TYPE} CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
    unset(NEW_CMAKE_BUILD_TYPE)
endif()

# Ensure CMAKE_BUILD_TYPE is set to Release or Debug when targeting Android as
# the NDK toolchain file does not support other build types
if(${MBP_BUILD_TARGET} STREQUAL android
        AND NOT ${CMAKE_BUILD_TYPE} STREQUAL Release
        AND NOT ${CMAKE_BUILD_TYPE} STREQUAL Debug)
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must be set to Release or Debug for the 'android' target")
endif()

# Sets:
# - MBP_TOP_LEVEL_BUILD: Whether this is a parent CMake build
# - MBP_INTERNAL_BUILD: Whether this is a recursive CMake build
# - MBP_TARGET_CONFIG_FILE: Config file for the current build target

if(${MBP_BUILD_TARGET} STREQUAL android)
    set(MBP_TARGET_CONFIG_FILE cmake/ConfigAndroid.cmake)
    set(MBP_TOP_LEVEL_BUILD TRUE)
    set(MBP_INTERNAL_BUILD FALSE)
elseif(${MBP_BUILD_TARGET} STREQUAL desktop)
    set(MBP_TARGET_CONFIG_FILE cmake/ConfigDesktop.cmake)
    set(MBP_TOP_LEVEL_BUILD TRUE)
    set(MBP_INTERNAL_BUILD FALSE)
elseif(${MBP_BUILD_TARGET} STREQUAL android-app)
    set(MBP_TARGET_CONFIG_FILE cmake/ConfigAndroidApp.cmake)
    set(MBP_TOP_LEVEL_BUILD FALSE)
    set(MBP_INTERNAL_BUILD TRUE)
elseif(${MBP_BUILD_TARGET} STREQUAL android-system)
    set(MBP_TARGET_CONFIG_FILE cmake/ConfigAndroidSystem.cmake)
    set(MBP_TOP_LEVEL_BUILD FALSE)
    set(MBP_INTERNAL_BUILD TRUE)
elseif(${MBP_BUILD_TARGET} STREQUAL hosttools)
    set(MBP_TARGET_CONFIG_FILE cmake/ConfigHostTools.cmake)
    set(MBP_TOP_LEVEL_BUILD FALSE)
    set(MBP_INTERNAL_BUILD TRUE)
endif()

# Screw Ubuntu's multiarch. FIND_LIBRARY_USE_LIB64_PATHS is disabled when
# /etc/debian_version exists, but we need lib64 in the library search path
# because the x86_64 NDK libraries are in:
#   <ndk>/platforms/android-21/arch-x86_64/usr/lib64/
if(${MBP_BUILD_TARGET} STREQUAL android-app
        OR ${MBP_BUILD_TARGET} STREQUAL android-system)
    set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)
endif()

# Code signing options
include(cmake/SigningConfigReader.cmake)
include(cmake/ConfigSigning.cmake)

# Build target config
include(${MBP_TARGET_CONFIG_FILE})

# Compile and link flags
include(cmake/CompilerFlags.cmake)

# Third party binaries
add_subdirectory(thirdparty)

# Dependencies
include(cmake/Dependencies.cmake)

set(MBP_GLOBAL_INCLUDE_DIRS
    libmbbootimg/include
    libmbcommon/include
    libmbdevice/include
    libmblog/include
    libmbpio/include
    libmbp/include
    libmbsign/include
    libmbsparse/include
    libmbutil/include
)

include_directories(${MBP_GLOBAL_INCLUDE_DIRS})

add_subdirectory(hosttools)
add_subdirectory(libmbcommon)
add_subdirectory(libmbbootimg)
add_subdirectory(libmbdevice)
add_subdirectory(libmblog)
add_subdirectory(libmbpio)
add_subdirectory(libmbp)
add_subdirectory(libmbsign)
add_subdirectory(libmbsparse)
add_subdirectory(libmbutil)
add_subdirectory(data)
add_subdirectory(mbtool)
add_subdirectory(mbbootui)
add_subdirectory(cryptfstool)
add_subdirectory(odinupdater)
add_subdirectory(misc)
add_subdirectory(libmiscstuff-jni)
add_subdirectory(Android_GUI)
add_subdirectory(gui)
add_subdirectory(bootimgtool)
add_subdirectory(examples)
add_subdirectory(utilities)
add_subdirectory(signtool)
add_subdirectory(devicesgen)
add_subdirectory(android)

# Must go after signtool since it references SIGNTOOL_COMMAND
configure_file(
    cmake/SignFiles.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/SignFiles.cmake
    @ONLY
)

include(CPack)
