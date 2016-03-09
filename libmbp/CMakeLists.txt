if(NOT MBP_TARGET_NEEDS_BUILDS)
    return()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(${MBP_BUILD_TARGET} STREQUAL android-app)
    include_directories(${MBP_LIBARCHIVE_LOWAPI_INCLUDES})
else()
    include_directories(${MBP_LIBARCHIVE_INCLUDES})
endif()
include_directories(${MBP_LZ4_INCLUDES})
include_directories(${MBP_LZO_INCLUDES})
include_directories(${MBP_LIBLZMA_INCLUDES})
include_directories(${MBP_ZLIB_INCLUDES})

include_directories(${CMAKE_SOURCE_DIR}/external)

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
    src/ramdiskpatchers/xperia.cpp
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

if(WIN32)
    list(APPEND MBP_SOURCES
        src/private/win32.cpp
    )
endif()

# minizip type safety
add_definitions(-DSTRICTZIPUNZIP)

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    # Build static library

    add_library(mbp-static STATIC ${MBP_SOURCES})

    target_compile_definitions(
        mbp-static
        PRIVATE
        -DMB_LIBRARY
    )

    if(NOT MSVC)
        set_target_properties(
            mbp-static
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
            C_STANDARD 99
            C_STANDARD_REQUIRED 1
        )
    endif()
else()
    # Build shared library

    add_library(mbp-shared SHARED ${MBP_SOURCES})

    target_compile_definitions(
        mbp-shared
        PRIVATE
        -DMB_DYNAMIC_LINK
        -DMB_LIBRARY
    )

    set_target_properties(
        mbp-shared
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        OUTPUT_NAME mbp
    )

    if(NOT MSVC)
        set_target_properties(
            mbp-shared
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
            C_STANDARD 99
            C_STANDARD_REQUIRED 1
        )
    endif()

    if(${MBP_BUILD_TARGET} STREQUAL android-app)
        set(LIBARCHIVE_LIBRARIES ${MBP_LIBARCHIVE_LOWAPI_LIBRARIES})
        set(ADDITIONAL_LIBRARIES c_compat)
    else()
        set(LIBARCHIVE_LIBRARIES ${MBP_LIBARCHIVE_LIBRARIES})
        set(ADDITIONAL_LIBRARIES)
    endif()

    target_link_libraries(
        mbp-shared
        mbpio-static
        mblog-shared
        minizip
        ${ADDITIONAL_LIBRARIES}
        ${LIBARCHIVE_LIBRARIES}
        ${MBP_ZLIB_LIBRARIES}
        ${MBP_LZ4_LIBRARIES}
        ${MBP_LZO_LIBRARIES}
        ${MBP_LIBLZMA_LIBRARIES}
    )

    if(UNIX AND NOT ANDROID)
        target_link_libraries(mbp-shared pthread)
    endif()

    # Install library
    install(
        TARGETS mbp-shared
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )
endif()
