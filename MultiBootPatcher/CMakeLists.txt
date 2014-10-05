cmake_minimum_required(VERSION 3.0)

project(DualBootPatcher)
set(DBP_VERSION_MAJOR 9)
set(DBP_VERSION_MINOR 0)
set(DBP_VERSION_PATCH 0Alpha6)
set(DBP_VERSION "${DBP_VERSION_MAJOR}.${DBP_VERSION_MINOR}.${DBP_VERSION_PATCH}")

include(ExternalProject)
include(GNUInstallDirs)

if(CMAKE_TOOLCHAIN_FILE)
    get_filename_component(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE} ABSOLUTE)
endif()

# Some cross-compiling toolchain files change the behavior of find_* and
# define find_host_* with the original behavior
if(NOT COMMAND find_host_package)
    macro(find_host_package)
        find_package(${ARGN})
    endmacro()
endif()
if(NOT COMMAND find_host_program)
    macro(find_host_program)
        find_program(${ARGN})
    endmacro()
endif()


# Enable C++11 support
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")
endif()


# Find dependencies
find_package(Qt5Core 5.3 REQUIRED)


# Portable application
option(PORTABLE "Build as portable application" OFF)

if(WIN32)
    set(PORTABLE ON CACHE BOOL "Build as portable application" FORCE)
endif()

if(${PORTABLE})
    set(BIN_INSTALL_DIR bin)
    set(DATA_INSTALL_DIR data)
    set(PLUGINS_INSTALL_DIR plugins)
    set(HEADERS_INSTALL_DIR include)

    if(WIN32)
        set(LIB_INSTALL_DIR bin)
    else()
        set(LIB_INSTALL_DIR lib)
    endif()
else()
    set(BIN_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
    set(DATA_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/dbp)
    set(PLUGINS_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/dbp)
    set(HEADERS_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
endif()


if(ANDROID)
# Allow the libarchive directory to be specified
list(APPEND CMAKE_FIND_ROOT_PATH ${LIBARCHIVE_DIR})
endif()

find_package(LibArchive REQUIRED)
include_directories(${LibArchive_INCLUDE_DIRS})
link_directories(${LibArchive_LIBRARY_DIRS})

# Need ndk-build for building the Android stuff
find_host_program(NDK_BUILD ndk-build HINTS "$ENV{ANDROID_NDK_HOME}" "$ENV{ANDROID_NDK}")
if(NOT NDK_BUILD)
    message(FATAL_ERROR "Could NOT find Android NDK (missing: ndk-build)")
endif()


# Main patcher
add_subdirectory(libdbp)
add_subdirectory(plugins)
add_subdirectory(data)
add_subdirectory(shortcuts)

if(NOT ANDROID)
    add_subdirectory(gui)
endif()

# Android stuff
add_subdirectory(syncdaemon)
add_subdirectory(libmountpoint)
add_subdirectory(libloki)

# Third party binaries
add_subdirectory(thirdparty)


# CPack stuff

# CPack removes the prefix when archive component install is used, unfortunately
#set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
#set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} Libraries Applications Plugins)

set(CPACK_PACKAGE_NAME "DualBootPatcher")
set(CPACK_PACKAGE_VERSION_MAJOR ${DBP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DBP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DBP_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A patcher for Android ROMs to make them dualbootable")

set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2;ZIP")

include(CPack)
