cmake_minimum_required(VERSION 3.8)

# Add our custom module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# Build target and type
set(MBP_BUILD_TARGET "device"
    CACHE STRING "Target platform (device, system, or hosttools)")
set(MBP_BUILD_TYPE "debug"
    CACHE STRING "Target build type (release, debug, or ci)")

# Build types:
# - device: Build for the target Android device
# - system: Build for the current system
# - hosttools: Builds tools needed on the host when cross-compiling
#              (not to be built manually)

# Verify build target and type
if(NOT ${MBP_BUILD_TARGET} STREQUAL device
        AND NOT ${MBP_BUILD_TARGET} STREQUAL system
        AND NOT ${MBP_BUILD_TARGET} STREQUAL hosttools)
    message(FATAL_ERROR "Invalid build target: ${MBP_BUILD_TARGET}")
endif()
if(NOT ${MBP_BUILD_TYPE} STREQUAL release
        AND NOT ${MBP_BUILD_TYPE} STREQUAL debug
        AND NOT ${MBP_BUILD_TYPE} STREQUAL ci)
    message(FATAL_ERROR "Invalid build type: ${MBP_BUILD_TYPE}")
endif()

# Set up cross compiling if targeting device
if(${MBP_BUILD_TARGET} STREQUAL device)
    if(NOT ANDROID_ABI)
        message(FATAL_ERROR "ANDROID_ABI must be set")
    endif()

    if(DEFINED ANDROID_NDK_HOME AND EXISTS "${ANDROID_NDK_HOME}")
        set(ndk_path "${ANDROID_NDK_HOME}")
    elseif(DEFINED ANDROID_NDK AND EXISTS "${ANDROID_NDK}")
        set(ndk_path "${ANDROID_NDK}")
    elseif(DEFINED ENV{ANDROID_NDK_HOME} AND EXISTS "$ENV{ANDROID_NDK_HOME}")
        set(ndk_path "$ENV{ANDROID_NDK_HOME}")
    elseif(DEFINED ENV{ANDROID_NDK} AND EXISTS "$ENV{ANDROID_NDK}")
        set(ndk_path "$ENV{ANDROID_NDK}")
    else()
        message(FATAL_ERROR "Could not find Android NDK")
    endif()

    # FIXME: Required until https://github.com/android-ndk/ndk/issues/222 is fixed
    set(ENV{ANDROID_NDK} "${ndk_path}")

    set(ANDROID_PIE OFF)
    set(ANDROID_PLATFORM android-28)
    set(ANDROID_STL c++_static)

    # ccache
    option(MBP_ANDROID_ENABLE_CCACHE "Enable use of ccache for Android" ON)
    if(MBP_ANDROID_ENABLE_CCACHE)
        find_program(CCACHE "ccache" DOC "Path to ccache binary")
        if(NOT CCACHE)
            message(FATAL_ERROR "MBP_ANDROID_ENABLE_CCACHE is enabled, but ccache was not found")
        endif()
        set(ANDROID_CCACHE ${CCACHE})
        unset(CCACHE)
    endif()

    # Load toolchain
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/android.toolchain.cmake")
endif()

project(DualBootPatcher CXX C ASM)
set(MBP_VERSION_MAJOR 9)
set(MBP_VERSION_MINOR 3)
set(MBP_VERSION_PATCH 0)
set(MBP_VERSION "${MBP_VERSION_MAJOR}.${MBP_VERSION_MINOR}.${MBP_VERSION_PATCH}")

# Allow version to be overridden in Jenkins
set(MBP_CI_VERSION "" CACHE STRING "Override version (continuous integration)")
if(MBP_CI_VERSION)
    if(NOT ${MBP_BUILD_TYPE} STREQUAL ci)
        message(FATAL_ERROR "Cannot override version number for non-CI builds")
    endif()
    set(MBP_VERSION ${MBP_CI_VERSION})
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

# Config file for the current build target
if(${MBP_BUILD_TARGET} STREQUAL device)
    set(MBP_TARGET_CONFIG_FILE cmake/ConfigDevice.cmake)
elseif(${MBP_BUILD_TARGET} STREQUAL system)
    set(MBP_TARGET_CONFIG_FILE cmake/ConfigSystem.cmake)
elseif(${MBP_BUILD_TARGET} STREQUAL hosttools)
    set(MBP_TARGET_CONFIG_FILE cmake/ConfigHostTools.cmake)
endif()

if(${MBP_BUILD_TARGET} STREQUAL device)
    # FIXME: Due to https://github.com/android-ndk/ndk/issues/222, we need a
    # custom toolchain file. Since we can't guarantee that the custom toolchain
    # file will work with newer versions of the NDK, we'll depend on an exact
    # version.
    set(ndk_version_whitelist
        r18.1.5063045
    )

    if(NOT ANDROID_NDK_RELEASE IN_LIST ndk_version_whitelist)
        message(FATAL_ERROR
                "Requires NDK version in list: ${ndk_version_whitelist}"
                " (detected: ${ANDROID_NDK_RELEASE})")
    endif()

    # Ubuntu's multiarch causes undesired behavior when cross-compiling.
    # FIND_LIBRARY_USE_LIB64_PATHS is disabled when /etc/debian_version exists,
    # but we need lib64 in the library search path because the x86_64 NDK
    # libraries are in:
    #   <ndk>/platforms/android-28/arch-x86_64/usr/lib64/
    set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)
endif()

# Code signing options
include(cmake/SigningConfigReader.cmake)
include(cmake/ConfigSigning.cmake)

# Build target config
include(${MBP_TARGET_CONFIG_FILE})

# Implicit system include directories
include(cmake/ImplicitIncludeDirs.cmake)

# Helper functions to backup/restore variables
include(cmake/VariableBackup.cmake)

# Compile and link flags
include(cmake/CompilerFlags.cmake)

# Tests
include(cmake/Testing.cmake)

# Third party binaries
add_subdirectory(thirdparty)

# Dependencies
include(cmake/Dependencies.cmake)

# QEMU-based testing
include(cmake/Qemu.cmake)

add_subdirectory(hosttools)
add_subdirectory(libmbcommon)
add_subdirectory(libmbbootimg)
add_subdirectory(libmbdevice)
add_subdirectory(libmblog)
add_subdirectory(libmbpio)
add_subdirectory(libmbsign)
add_subdirectory(libmbsparse)
add_subdirectory(libmbsystrace)
add_subdirectory(libmbutil)
add_subdirectory(mbtool)
add_subdirectory(app)
add_subdirectory(odinupdater)
add_subdirectory(misc)
add_subdirectory(bootimgtool)
add_subdirectory(examples)
add_subdirectory(signtool)

# Must go last because install() statements specified directly in this come
# before all subdirectories in cmake_install.cmake.
add_subdirectory(cmake/postinstall)

include(CPack)
