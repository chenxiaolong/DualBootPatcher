include(ExternalProject)

set(MBLOG_SOURCES
    src/logging.cpp
    src/stdio_logger.cpp
)

set(MBLOG_ANDROID_SOURCES
    src/android_logger.cpp
    src/kmsg_logger.cpp
)

if(${MBP_BUILD_TARGET} STREQUAL desktop)
    add_library(mblog SHARED ${MBLOG_SOURCES})

    set_target_properties(
        mblog
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    target_compile_definitions(
        mblog
        PRIVATE
        -DDMB_DYNAMIC_LINK
        -DMB_LIBRARY
    )

    if(NOT MSVC)
        set_target_properties(
            mblog
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Install library
    install(
        TARGETS mblog
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )
endif()

set(MBLOG_SOURCES_STR "")
foreach(_source ${MBLOG_SOURCES} ${MBLOG_ANDROID_SOURCES})
    set(MBLOG_SOURCES_STR "${MBLOG_SOURCES_STR} ${_source}")
endforeach()

# Generate Android.mk
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.shared.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.shared.mk
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.static.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.static.mk
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.shared.include.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.shared.include.mk
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.static.include.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.static.include.mk
    @ONLY
)

if(${MBP_BUILD_TARGET} STREQUAL android)
    ExternalProject_Add(
        mblog-shared-android
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out.shared/
        BUILD_COMMAND ${NDK_BUILD}
                      -C <SOURCE_DIR>
                      NDK_PROJECT_PATH=.
                      NDK_APPLICATION_MK=Application.mk
                      NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out.shared
                      NDK_OUT=<TMP_DIR>
                      APP_BUILD_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/Android.shared.mk
                      ${NDK_ARGS}
        INSTALL_COMMAND ""
    )

    # Always rebuild since files may have changed
    ExternalProject_Add_Step(
        mblog-shared-android
        forcerebuild
        DEPENDEES update
        DEPENDERS configure
        ALWAYS 1
        COMMAND ""
    )
endif()

ExternalProject_Add(
    mblog-static-android
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out.static/
    BUILD_COMMAND ${NDK_BUILD}
                  -C <SOURCE_DIR>
                  NDK_PROJECT_PATH=.
                  NDK_APPLICATION_MK=Application.mk
                  NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out.static
                  NDK_OUT=<TMP_DIR>
                  APP_BUILD_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/Android.static.mk
                  ${NDK_ARGS}
    INSTALL_COMMAND ""
)

# Always rebuild since files may have changed
ExternalProject_Add_Step(
    mblog-static-android
    forcerebuild
    DEPENDEES update
    DEPENDERS configure
    ALWAYS 1
    COMMAND ""
)
