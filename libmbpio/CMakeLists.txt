include(ExternalProject)

# Allow libmbpio headers to be found
include_directories(${CMAKE_SOURCE_DIR})

set(MBP_IO_SOURCES
    delete.cpp
    directory.cpp
    error.cpp
    path.cpp
    private/utf8.cpp
    private/filebase.cpp
    private/string.cpp
)

if(WIN32)
    set(MBP_IO_SOURCES
        ${MBP_IO_SOURCES}
        win32/delete.cpp
        win32/error.cpp
        win32/file.cpp
    )
else()
    set(MBP_IO_SOURCES
        ${MBP_IO_SOURCES}
        posix/delete.cpp
        posix/file.cpp
    )
endif()

if(NOT MBP_BUILD_ANDROID)
    # For desktop machines, we'll build with cmake

    add_library(mbpio STATIC ${MBP_IO_SOURCES})

    set_target_properties(
        mbpio
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    if(NOT MSVC)
        set_target_properties(
            mbpio
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()
endif()


################################################################################
# libmbpio for Android
################################################################################

set(MBP_IO_ANDROID_SOURCES
    delete.cpp
    directory.cpp
    error.cpp
    path.cpp
    android/file.cpp
    posix/delete.cpp
    private/utf8.cpp
    private/filebase.cpp
    private/string.cpp
)

# Wordaround semicolon separated list issue
set(MBP_IO_SOURCES_STR "")
foreach(_source ${MBP_IO_ANDROID_SOURCES})
    set(MBP_IO_SOURCES_STR "${MBP_IO_SOURCES_STR} ${_source}")
endforeach()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.mk
    @ONLY
)

ExternalProject_Add(
    mbpio-android
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out/
    BUILD_COMMAND ${NDK_BUILD}
                  -C <SOURCE_DIR>
                  -j${PROCESSOR_COUNT}
                  NDK_PROJECT_PATH=.
                  NDK_APPLICATION_MK=Application.mk
                  NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out/
                  NDK_OUT=<TMP_DIR>
                  APP_BUILD_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/Android.mk
                  ${NDK_ARGS}
    INSTALL_COMMAND ""
)

# Always rebuild since files may have changed
ExternalProject_Add_Step(
    mbpio-android
    forcerebuild
    DEPENDEES update
    DEPENDERS configure
    ALWAYS 1
    COMMAND ""
)
