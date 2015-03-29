include(ExternalProject)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Allow libmbp headers to be found
include_directories(${CMAKE_SOURCE_DIR})

include_directories(${MBP_ZLIB_INCLUDES})
include_directories(${MBP_LIBLZMA_INCLUDES})
include_directories(${MBP_LIBARCHIVE_INCLUDES})

include_directories(${CMAKE_SOURCE_DIR}/external)
include_directories(${CMAKE_SOURCE_DIR}/external/pugixml/src)

set(MBP_SOURCES
    bootimage.cpp
    cpiofile.cpp
    device.cpp
    fileinfo.cpp
    patcherconfig.cpp
    patchererror.cpp
    patchinfo.cpp
    private/fileutils.cpp
    private/logging.cpp
    bootimage/bumppatcher.cpp
    bootimage/lokipatcher.cpp
    cwrapper/cbootimage.cpp
    cwrapper/ccommon.cpp
    cwrapper/ccpiofile.cpp
    cwrapper/cdevice.cpp
    cwrapper/cfileinfo.cpp
    cwrapper/cpatcherconfig.cpp
    cwrapper/cpatchererror.cpp
    cwrapper/cpatcherinterface.cpp
    cwrapper/cpatchinfo.cpp
    cwrapper/cstringmap.cpp
    cwrapper/private/util.cpp
    # Autopatchers
    autopatchers/patchfilepatcher.cpp
    autopatchers/standardpatcher.cpp
    # Patchers
    patchers/mbtoolupdater.cpp
    patchers/multibootpatcher.cpp
    # Ramdisk patchers
    ramdiskpatchers/baconramdiskpatcher.cpp
    ramdiskpatchers/coreramdiskpatcher.cpp
    ramdiskpatchers/falconramdiskpatcher.cpp
    ramdiskpatchers/floramdiskpatcher.cpp
    ramdiskpatchers/galaxyramdiskpatcher.cpp
    ramdiskpatchers/hammerheadramdiskpatcher.cpp
    ramdiskpatchers/hlteramdiskpatcher.cpp
    ramdiskpatchers/jflteramdiskpatcher.cpp
    ramdiskpatchers/klteramdiskpatcher.cpp
    ramdiskpatchers/lgg2ramdiskpatcher.cpp
    ramdiskpatchers/qcomramdiskpatcher.cpp
    ramdiskpatchers/trlteramdiskpatcher.cpp
    external/sha.c
    ${CMAKE_SOURCE_DIR}/external/cppformat/format.cc
    ${CMAKE_SOURCE_DIR}/external/pugixml/src/pugixml.cpp
)

set(MBP_HEADERS
    bootimage.h
    cpiofile.h
    device.h
    fileinfo.h
    libmbp_global.h
    logging.h
    patcherconfig.h
    patchererror.h
    patcherinterface.h
    patchinfo.h
    cwrapper/cbootimage.h
    cwrapper/ccommon.h
    cwrapper/ccpiofile.h
    cwrapper/cdevice.h
    cwrapper/cfileinfo.h
    cwrapper/cpatcherconfig.h
    cwrapper/cpatchererror.h
    cwrapper/cpatcherinterface.h
    cwrapper/cpatchinfo.h
    cwrapper/cstringmap.h
    cwrapper/ctypes.h
)

set(MBP_NOINST_HEADERS
    # Autopatchers
    autopatchers/patchfilepatcher.h
    autopatchers/standardpatcher.h
    # Patchers
    patchers/mbtoolupdater.h
    patchers/multibootpatcher.h
    # Ramdisk patchers
    ramdiskpatchers/baconramdiskpatcher.h
    ramdiskpatchers/coreramdiskpatcher.h
    ramdiskpatchers/falconramdiskpatcher.h
    ramdiskpatchers/floramdiskpatcher.h
    ramdiskpatchers/galaxyramdiskpatcher.h
    ramdiskpatchers/hammerheadramdiskpatcher.h
    ramdiskpatchers/hlteramdiskpatcher.h
    ramdiskpatchers/jflteramdiskpatcher.h
    ramdiskpatchers/klteramdiskpatcher.h
    ramdiskpatchers/lgg2ramdiskpatcher.h
    ramdiskpatchers/qcomramdiskpatcher.h
    ramdiskpatchers/trlteramdiskpatcher.h
)

# If we're building for Android, then compile with ndk-build since it can easily
# handle building for multiple architectures

if(MBP_BUILD_ANDROID)
    # Wordaround semicolon separated list issue
    set(MBP_SOURCES_STR "")
    foreach(_source ${MBP_SOURCES})
        set(MBP_SOURCES_STR "${MBP_SOURCES_STR} ${_source}")
    endforeach()

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk.in
        ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk
        @ONLY
    )

    ExternalProject_Add(
        mbp
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out/
        BUILD_COMMAND ${NDK_BUILD}
                      -C <SOURCE_DIR>
                      -j${PROCESSOR_COUNT}
                      NDK_PROJECT_PATH=.
                      NDK_APPLICATION_MK=Application.mk
                      NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out
                      NDK_OUT=<TMP_DIR>
                      APP_BUILD_SCRIPT=Android.mk
                      ${NDK_ARGS}
        INSTALL_COMMAND ""
    )

    # Always rebuild since files may have changed
    ExternalProject_Add_Step(
        mbp
        forcerebuild
        DEPENDEES update
        DEPENDERS configure
        ALWAYS 1
        COMMAND ""
    )
else()
    # For desktop machines, we'll build with cmake

    add_definitions(
        -DLIBMBP_LIBRARY
        -DLIBMBP_VERSION="${MBP_VERSION}"
    )

    add_library(mbp SHARED ${MBP_SOURCES})

    set_target_properties(mbp PROPERTIES
        VERSION 1.0.0
        SOVERSION 1
    )

    if(NOT MSVC)
        set_target_properties(mbp PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
            C_STANDARD 99
            C_STANDARD_REQUIRED 1
        )
    endif()

    target_link_libraries(mbp
        ${Boost_LIBRARIES}
        ${MBP_ZLIB_LIBRARIES}
        ${MBP_LIBLZMA_LIBRARIES}
        ${MBP_LIBARCHIVE_LIBRARIES}
    )

    if(WIN32)
        target_link_libraries(mbp ws2_32) # For htonl()
    endif()

    # Install library
    install(
        TARGETS mbp
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )

    # Install headers
    install(
        FILES ${MBP_HEADERS}
        DESTINATION ${HEADERS_INSTALL_DIR}/libmbp
        COMPONENT Development
    )
endif()


################################################################################
# libmbp-mini for Android
################################################################################

set(MBP_MINI_SOURCES
    bootimage.cpp
    cpiofile.cpp
    device.cpp
    patcherconfig.cpp
    patchererror.cpp
    #patchinfo.cpp
    private/fileutils.cpp
    private/logging.cpp
    cwrapper/cbootimage.cpp
    cwrapper/ccommon.cpp
    cwrapper/ccpiofile.cpp
    cwrapper/cdevice.cpp
    cwrapper/cpatcherconfig.cpp
    cwrapper/cpatchererror.cpp
    #cwrapper/cpatchinfo.cpp
    cwrapper/cstringmap.cpp
    cwrapper/private/util.cpp
    bootimage/bumppatcher.cpp
    bootimage/lokipatcher.cpp
    external/sha.c
    ${CMAKE_SOURCE_DIR}/external/cppformat/format.cc
)

# Wordaround semicolon separated list issue
set(MBP_SOURCES_STR "")
foreach(_source ${MBP_MINI_SOURCES})
    set(MBP_SOURCES_STR "${MBP_SOURCES_STR} ${_source}")
endforeach()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk.in
    ${CMAKE_CURRENT_SOURCE_DIR}/Android-mini.mk
    @ONLY
)

ExternalProject_Add(
    mbp-mini
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out-mini/
    BUILD_COMMAND ${NDK_BUILD}
                  -C <SOURCE_DIR>
                  -j${PROCESSOR_COUNT}
                  NDK_PROJECT_PATH=.
                  NDK_APPLICATION_MK=Application.mk
                  NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out-mini
                  NDK_OUT=<TMP_DIR>
                  APP_BUILD_SCRIPT=Android-mini.mk
                  ${NDK_ARGS}
                  MBP_MINI=true
    INSTALL_COMMAND ""
)

# Always rebuild since files may have changed
ExternalProject_Add_Step(
    mbp-mini
    forcerebuild
    DEPENDEES update
    DEPENDERS configure
    ALWAYS 1
    COMMAND ""
)

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out-mini/
    DESTINATION ${DATA_INSTALL_DIR}/libraries/
    COMPONENT Libraries
    FILES_MATCHING
    PATTERN libmbp-mini.so
)
