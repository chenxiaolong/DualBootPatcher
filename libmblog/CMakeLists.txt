include(ExternalProject)

include_directories(include)

set(MBLOG_SOURCES
    src/logging.cpp
    src/stdio_logger.cpp
)

set(MBLOG_ANDROID_SOURCES
    src/android_logger.cpp
    src/kmsg_logger.cpp
)

if(${MBP_BUILD_TARGET} STREQUAL desktop)
    add_library(mblog STATIC ${MBLOG_SOURCES})

    set_target_properties(
        mblog
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    if(NOT MSVC)
        set_target_properties(
            mblog
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()
endif()

set(MBLOG_SOURCES_STR "")
foreach(_source ${MBLOG_SOURCES} ${MBLOG_ANDROID_SOURCES})
    set(MBLOG_SOURCES_STR "${MBLOG_SOURCES_STR} ${_source}")
endforeach()

# Generate Android.mk
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.mk
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.include.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.include.mk
    @ONLY
)

ExternalProject_Add(
    mblog-android
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out/
    BUILD_COMMAND ${NDK_BUILD}
                  -C <SOURCE_DIR>
                  NDK_PROJECT_PATH=.
                  NDK_APPLICATION_MK=Application.mk
                  NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out
                  NDK_OUT=<TMP_DIR>
                  APP_BUILD_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/Android.mk
                  ${NDK_ARGS}
    INSTALL_COMMAND ""
)

# Always rebuild since files may have changed
ExternalProject_Add_Step(
    mblog-android
    forcerebuild
    DEPENDEES update
    DEPENDERS configure
    ALWAYS 1
    COMMAND ""
)