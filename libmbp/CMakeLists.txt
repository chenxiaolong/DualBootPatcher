include(ExternalProject)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Allow libmbp headers to be found
include_directories(${CMAKE_SOURCE_DIR})

include_directories(${MBP_ZLIB_INCLUDES})
include_directories(${MBP_LIBLZMA_INCLUDES})
include_directories(${MBP_LIBARCHIVE_INCLUDES})

include_directories(${CMAKE_SOURCE_DIR}/external)

include_directories(${CMAKE_SOURCE_DIR}/libmbcommon/include)
include_directories(${CMAKE_SOURCE_DIR}/libmblog/include)
include_directories(${CMAKE_SOURCE_DIR}/libmbpio/include)

include_directories(include)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mbp/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/mbp/version.h
    @ONLY
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

set(MBP_CORE_SOURCES
    src/bootimage.cpp
    src/cpiofile.cpp
    src/device.cpp
    src/fileinfo.cpp
    src/patcherconfig.cpp
    # Boot image parsers
    src/bootimage/androidformat.cpp
    src/bootimage/bumpformat.cpp
    src/bootimage/bumppatcher.cpp
    src/bootimage/fileformat.cpp
    src/bootimage/lokiformat.cpp
    src/bootimage/lokipatcher.cpp
    src/bootimage/mtkformat.cpp
    src/bootimage/sonyelfformat.cpp
    # C wrapper API
    src/cwrapper/cbootimage.cpp
    src/cwrapper/ccommon.cpp
    src/cwrapper/ccpiofile.cpp
    src/cwrapper/cdevice.cpp
    src/cwrapper/cfileinfo.cpp
    src/cwrapper/cpatcherconfig.cpp
    src/cwrapper/cpatcherinterface.cpp
    src/cwrapper/private/util.cpp
    # Edify tokenizer
    src/edify/tokenizer.cpp
    # Private classes
    src/private/fileutils.cpp
    src/private/miniziputils.cpp
    src/private/stringutils.cpp
    # Autopatchers
    src/autopatchers/standardpatcher.cpp
    src/autopatchers/xposedpatcher.cpp
    # Patchers
    src/patchers/mbtoolupdater.cpp
    src/patchers/multibootpatcher.cpp
    src/patchers/odinpatcher.cpp
    # Ramdisk patchers
    src/ramdiskpatchers/core.cpp
    src/ramdiskpatchers/default.cpp
    src/ramdiskpatchers/pepper.cpp
    # SHA1 library
    external/sha.cpp
)

set(MBP_DEVICE_SOURCES
    devices/asus.cpp
    devices/google.cpp
    devices/huawei.cpp
    devices/lenovo.cpp
    devices/lg.cpp
    devices/motorola.cpp
    devices/nexus.cpp
    devices/oneplus.cpp
    devices/samsung.cpp
    devices/sony.cpp
    devices/xiaomi.cpp
)

set(MBP_SOURCES
    ${MBP_CORE_SOURCES}
    ${MBP_DEVICE_SOURCES}
)

set(MBP_ANDROID_SOURCES
    ${MBP_SOURCES}
)

if(WIN32)
    list(APPEND MBP_SOURCES
        src/private/win32.cpp
    )
endif()

# Wordaround semicolon separated list issue
set(MBP_SOURCES_STR "")
foreach(_source ${MBP_ANDROID_SOURCES})
    set(MBP_SOURCES_STR "${MBP_SOURCES_STR} ${_source}")
endforeach()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.common.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.common.mk
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.include.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.include.mk
    @ONLY
)
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

# If we're building for Android, then compile with ndk-build since it can easily
# handle building for multiple architectures
if(${MBP_BUILD_TARGET} STREQUAL android)
    ExternalProject_Add(
        mbp-shared-android
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out.shared/
        BUILD_COMMAND ${NDK_BUILD}
                      -C <SOURCE_DIR>
                      NDK_PROJECT_PATH=.
                      NDK_APPLICATION_MK=Application.shared.mk
                      NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out.shared
                      NDK_OUT=<TMP_DIR>
                      APP_BUILD_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/Android.shared.mk
                      ${NDK_ARGS}
        INSTALL_COMMAND ""
    )

    # Always rebuild since files may have changed
    ExternalProject_Add_Step(
        mbp-shared-android
        forcerebuild
        DEPENDEES update
        DEPENDERS configure
        ALWAYS 1
        COMMAND ""
    )

    add_dependencies(
        mbp-shared-android
        mbpio-android
    )
else()
    # For desktop machines, we'll build with cmake

    add_definitions(
        -DMB_DYNAMIC_LINK
        -DMB_LIBRARY
        -DSTRICTZIPUNZIP
    )

    add_library(mbp SHARED ${MBP_SOURCES})

    if(NOT MSVC)
        set_target_properties(mbp PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
            C_STANDARD 99
            C_STANDARD_REQUIRED 1
        )
    endif()

    target_link_libraries(mbp
        mbpio
        mblog
        ${MBP_ZLIB_LIBRARIES}
        ${MBP_LIBLZMA_LIBRARIES}
        ${MBP_LIBARCHIVE_LIBRARIES}
        minizip
    )

    if(UNIX)
        target_link_libraries(mbp pthread)
    endif()

    # Install library
    install(
        TARGETS mbp
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )
endif()


################################################################################
# libmbp-static for Android
################################################################################

ExternalProject_Add(
    mbp-static-android
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out.static/
    BUILD_COMMAND ${NDK_BUILD}
                  -C <SOURCE_DIR>
                  NDK_PROJECT_PATH=.
                  NDK_APPLICATION_MK=Application.static.mk
                  NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out.static
                  NDK_OUT=<TMP_DIR>
                  APP_BUILD_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/Android.static.mk
                  ${NDK_ARGS}
    INSTALL_COMMAND ""
)

# Always rebuild since files may have changed
ExternalProject_Add_Step(
    mbp-static-android
    forcerebuild
    DEPENDEES update
    DEPENDERS configure
    ALWAYS 1
    COMMAND ""
)

add_dependencies(
    mbp-static-android
    mbpio-android
    mblog-android
)
