set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Allow libmbp headers to be found
include_directories(${CMAKE_SOURCE_DIR})

include_directories(jsoncpp-dist)

set(MBP_SOURCES
    bootimage.cpp
    cpiofile.cpp
    device.cpp
    fileinfo.cpp
    partitionconfig.cpp
    patcherconfig.cpp
    patchererror.cpp
    patchinfo.cpp
    private/fileutils.cpp
    cwrapper/cbootimage.cpp
    cwrapper/ccommon.cpp
    cwrapper/ccpiofile.cpp
    cwrapper/cdevice.cpp
    cwrapper/cfileinfo.cpp
    cwrapper/cpartitionconfig.cpp
    cwrapper/cpatcherconfig.cpp
    cwrapper/cpatchererror.cpp
    cwrapper/cpatcherinterface.cpp
    cwrapper/cpatchinfo.cpp
    cwrapper/cstringmap.cpp
    # Autopatchers
    autopatchers/jflte/jfltepatcher.cpp
    autopatchers/patchfile/patchfilepatcher.cpp
    autopatchers/standard/standardpatcher.cpp
    # Patchers
    patchers/multiboot/multibootpatcher.cpp
    patchers/primaryupgrade/primaryupgradepatcher.cpp
    patchers/syncdaemonupdate/syncdaemonupdatepatcher.cpp
    # Ramdisk patchers
    ramdiskpatchers/bacon/baconramdiskpatcher.cpp
    ramdiskpatchers/common/coreramdiskpatcher.cpp
    ramdiskpatchers/d800/d800ramdiskpatcher.cpp
    ramdiskpatchers/falcon/falconramdiskpatcher.cpp
    ramdiskpatchers/flo/floramdiskpatcher.cpp
    ramdiskpatchers/galaxy/galaxyramdiskpatcher.cpp
    ramdiskpatchers/hammerhead/hammerheadramdiskpatcher.cpp
    ramdiskpatchers/hlte/hlteramdiskpatcher.cpp
    ramdiskpatchers/jflte/jflteramdiskpatcher.cpp
    ramdiskpatchers/klte/klteramdiskpatcher.cpp
    ramdiskpatchers/qcom/qcomramdiskpatcher.cpp
    # jsoncpp
    jsoncpp-dist/jsoncpp.cpp
)

set(MBP_HEADERS
    bootimage.h
    cpiofile.h
    device.h
    fileinfo.h
    libmbp_global.h
    partitionconfig.h
    patcherconfig.h
    patchererror.h
    patcherinterface.h
    patchinfo.h
)

set(MBP_NOINST_HEADERS
    # Autopatchers
    autopatchers/jflte/jfltepatcher.h
    autopatchers/patchfile/patchfilepatcher.h
    autopatchers/standard/standardpatcher.h
    # Patchers
    patchers/multiboot/multibootpatcher.h
    patchers/primaryupgrade/primaryupgradepatcher.h
    patchers/syncdaemonupdate/syncdaemonupdatepatcher.h
    # Ramdisk patchers
    ramdiskpatchers/bacon/baconramdiskpatcher.h
    ramdiskpatchers/common/coreramdiskpatcher.h
    ramdiskpatchers/d800/d800ramdiskpatcher.h
    ramdiskpatchers/falcon/falconramdiskpatcher.h
    ramdiskpatchers/flo/floramdiskpatcher.h
    ramdiskpatchers/galaxy/galaxyramdiskpatcher.h
    ramdiskpatchers/hammerhead/hammerheadramdiskpatcher.h
    ramdiskpatchers/hlte/hlteramdiskpatcher.h
    ramdiskpatchers/jflte/jflteramdiskpatcher.h
    ramdiskpatchers/klte/klteramdiskpatcher.h
    ramdiskpatchers/qcom/qcomramdiskpatcher.h
)

# If we're building for Android, then compile with ndk-build since it can easily
# handle building for multiple architectures

if(BUILD_ANDROID)
    include(ExternalProject)
    include(${CMAKE_SOURCE_DIR}/cmake/LogicalCoreCount.cmake)

    # Wordaround semicolon separated list issue
    set(MBP_SOURCES_STR "")
    foreach(_source ${MBP_SOURCES})
        set(MBP_SOURCES_STR "${MBP_SOURCES_STR} ${_source}")
    endforeach()

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk.in
        #${CMAKE_CURRENT_BINARY_DIR}/Android.mk
        ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk
        @ONLY
    )

    ExternalProject_Add(
        mbp
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/libmbp
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/libmbp/out/
        BUILD_COMMAND ${NDK_BUILD}
                      -C <SOURCE_DIR>
                      -j${PROCESSOR_COUNT}
                      #APP_BUILD_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/Android.mk
                      NDK_PROJECT_PATH=.
                      NDK_APPLICATION_MK=Application.mk
                      NDK_APP_LIBS_OUT=${CMAKE_BINARY_DIR}/libmbp/out
                      NDK_OUT=<TMP_DIR>
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

    target_link_libraries(mbp
        ${Boost_LIBRARIES}
        ${LibArchive_LIBRARIES}
        ${LIBXML2_LIBRARIES}
    )

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
