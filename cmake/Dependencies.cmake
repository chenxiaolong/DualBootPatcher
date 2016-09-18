# Dependencies

if(MBP_TOP_LEVEL_BUILD)
    option(MBP_ANDROID_ENABLE_CCACHE "Enable use of ccache for Android" ON)

    if(MBP_ANDROID_ENABLE_CCACHE)
        find_program(CCACHE "ccache" DOC "Path to ccache binary")
        if(NOT CCACHE)
            message(FATAL_ERROR "MBP_ANDROID_ENABLE_CCACHE is enabled, but ccache was not found")
        endif()
        set(MBP_CCACHE_PATH ${CCACHE})
        unset(CCACHE)
    endif()
endif()

# Don't need dependencies for Android top-level build
if(${MBP_BUILD_TARGET} STREQUAL android)
    return()
endif()

if(NOT ANDROID)
    # Qt5
    if (${MBP_BUILD_TARGET} STREQUAL desktop)
        find_package(Qt5Core 5.3 REQUIRED)
        find_package(Qt5Widgets 5.3 REQUIRED)
    endif()

    # GTest
    if(MBP_ENABLE_TESTS)
        find_package(GTest REQUIRED)
    endif()

    # Same logic as CMakeLists.txt from the CMake source
    set(EXTERNAL_LIBRARIES LIBARCHIVE)
    foreach(extlib ${EXTERNAL_LIBRARIES})
        if(NOT DEFINED MBP_USE_SYSTEM_LIBRARY_${extlib}
                AND DEFINED MBP_USE_SYSTEM_LIBRARIES)
            set(MBP_USE_SYSTEM_LIBRARY_${extlib} "${MBP_USE_SYSTEM_LIBRARIES}")
        endif()
        if(DEFINED MBP_USE_SYSTEM_LIBRARY_${extlib})
            if(MBP_USE_SYSTEM_LIBRARY_${extlib})
                set(MBP_USE_SYSTEM_LIBRARY_${extlib} ON)
            else()
                set(MBP_USE_SYSTEM_LIBRARY_${extlib} OFF)
            endif()
            string(TOLOWER "${extlib}" extlib_lower)
            set(MBP_USE_SYSTEM_${extlib} "${MBP_USE_SYSTEM_LIBRARY_${extlib}}"
                CACHE BOOL "Use system-installed ${extlib_lower}" FORCE)
        else()
            set(MBP_USE_SYSTEM_LIBRARY_${extlib} OFF)
        endif()
    endforeach()

    # Optionally use system utility libraries.
    option(
        MBP_USE_SYSTEM_LIBARCHIVE
        "Use system-installed libarchive"
        "${MBP_USE_SYSTEM_LIBRARY_LIBARCHIVE}"
    )

    foreach(extlib ${EXTERNAL_LIBRARIES})
        if(MBP_USE_SYSTEM_${extlib})
            message(STATUS "Using system-installed ${extlib}")
        endif()
    endforeach()

    # Prefer static libraries when compiling with mingw
    option(
        MBP_MINGW_USE_STATIC_LIBS
        "Prefer static libraries when compiling with mingw"
        OFF
    )
    if(WIN32 AND MINGW AND MBP_MINGW_USE_STATIC_LIBS)
        set(CMAKE_FIND_LIBRARY_SUFFIXES_OLD ${CMAKE_FIND_LIBRARY_SUFFIXES})
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
endif()

if(${MBP_BUILD_TARGET} STREQUAL hosttools)
    include(cmake/dependencies/yaml-cpp.cmake)
else()
    include(cmake/dependencies/zlib.cmake)
    include(cmake/dependencies/liblzma.cmake)
    include(cmake/dependencies/lz4.cmake)
    include(cmake/dependencies/lzo.cmake)
    include(cmake/dependencies/libarchive.cmake)
    include(cmake/dependencies/libsepol.cmake)
    include(cmake/dependencies/fuse.cmake)
    include(cmake/dependencies/procps-ng.cmake)
    include(cmake/dependencies/minizip.cmake)
    include(cmake/dependencies/libpng.cmake)
    include(cmake/dependencies/freetype2.cmake)
    include(cmake/dependencies/libdrm.cmake)
    include(cmake/dependencies/safe-iop.cmake)
    include(cmake/dependencies/android-system-core.cmake)
endif()

include(cmake/dependencies/jansson.cmake)
include(cmake/dependencies/openssl.cmake)

if(NOT ANDROID)
    # Restore CMAKE_FIND_LIBRARY_SUFFIXES
    if(WIN32 AND MINGW AND MBP_MINGW_USE_STATIC_LIBS)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_OLD})
        unset(CMAKE_FIND_LIBRARY_SUFFIXES_OLD)
    endif()
endif()
