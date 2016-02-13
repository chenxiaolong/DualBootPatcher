include(ExternalProject)

set(MBP_IO_SOURCES
    src/delete.cpp
    src/directory.cpp
    src/error.cpp
    src/path.cpp
    src/private/utf8.cpp
    src/private/filebase.cpp
    src/private/string.cpp
)

if(WIN32)
    set(MBP_IO_SOURCES
        ${MBP_IO_SOURCES}
        src/win32/delete.cpp
        src/win32/error.cpp
        src/win32/file.cpp
    )
else()
    set(MBP_IO_SOURCES
        ${MBP_IO_SOURCES}
        src/posix/delete.cpp
        src/posix/file.cpp
    )
endif()

if(${MBP_BUILD_TARGET} STREQUAL desktop)
    # For desktop machines, we'll build with cmake

    add_library(mbpio SHARED ${MBP_IO_SOURCES})

    target_compile_definitions(
        mbpio
        PRIVATE
        -DDMB_DYNAMIC_LINK
        -DMB_LIBRARY
    )

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

    # Install library
    install(
        TARGETS mbpio
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )
endif()


################################################################################
# libmbpio for Android
################################################################################

set(MBP_IO_ANDROID_SOURCES
    src/delete.cpp
    src/directory.cpp
    src/error.cpp
    src/path.cpp
    src/android/file.cpp
    src/posix/delete.cpp
    src/private/utf8.cpp
    src/private/filebase.cpp
    src/private/string.cpp
)

# Wordaround semicolon separated list issue
set(MBP_IO_SOURCES_STR "")
foreach(_source ${MBP_IO_ANDROID_SOURCES})
    set(MBP_IO_SOURCES_STR "${MBP_IO_SOURCES_STR} ${_source}")
endforeach()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.static.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.static.mk
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.static.include.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.static.include.mk
    @ONLY
)

ExternalProject_Add(
    mbpio-static-android
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out/
    BUILD_COMMAND ${NDK_BUILD}
                  -C <SOURCE_DIR>
                  NDK_PROJECT_PATH=.
                  NDK_APPLICATION_MK=Application.mk
                  NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out/
                  NDK_OUT=<TMP_DIR>
                  APP_BUILD_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/Android.static.mk
                  ${NDK_ARGS}
    INSTALL_COMMAND ""
)

# Always rebuild since files may have changed
ExternalProject_Add_Step(
    mbpio-static-android
    forcerebuild
    DEPENDEES update
    DEPENDERS configure
    ALWAYS 1
    COMMAND ""
)
