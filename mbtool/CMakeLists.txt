if(NOT ${MBP_BUILD_TARGET} STREQUAL android-system)
    return()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${MBP_JANSSON_INCLUDES})
include_directories(${MBP_LIBARCHIVE_INCLUDES})
include_directories(${MBP_LIBSEPOL_INCLUDES})
include_directories(${MBP_OPENSSL_INCLUDES})
include_directories(${MBP_PROCPS_NG_INCLUDES})
include_directories(${CMAKE_SOURCE_DIR}/external)
include_directories(${CMAKE_SOURCE_DIR}/external/flatbuffers/include)
include_directories(${CMAKE_SOURCE_DIR}/external/pugixml/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/linux-api-headers)

# Generate validcerts.cpp
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/validcerts.cpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/gen/validcerts.cpp
)

set(MINIADBD_SOURCES
    miniadbd/adb.cpp
    miniadbd/adb_io.cpp
    miniadbd/adb_log.cpp
    miniadbd/adb_utils.cpp
    miniadbd/fdevent.cpp
    miniadbd/file_sync_service.cpp
    miniadbd/services.cpp
    miniadbd/sockets.cpp
    miniadbd/transport.cpp
    miniadbd/transport_usb.cpp
    miniadbd/usb_linux_client.cpp
)

set(MBTOOL_BASE_SOURCES
    appsync.cpp
    appsyncmanager.cpp
    auditd.cpp
    daemon.cpp
    daemon_v3.cpp
    emergency.cpp
    init.cpp
    main.cpp
    miniadbd.cpp
    mount_fstab.cpp
    multiboot.cpp
    packages.cpp
    properties.cpp
    reboot.cpp
    romconfig.cpp
    roms.cpp
    sepolpatch.cpp
    signature.cpp
    switcher.cpp
    uevent_dump.cpp
    wipe.cpp
    external/legacy_property_service.cpp
    external/audit/libaudit.cpp
    external/property_service.cpp
    initwrapper/cutils/uevent.cpp
    initwrapper/devices.cpp
    initwrapper/util.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/gen/validcerts.cpp
    ${CMAKE_SOURCE_DIR}/external/pugixml/src/pugixml.cpp
)

set(MBTOOL_RECOVERY_SOURCES
    archive_util.cpp
    backup.cpp
    bootimg_util.cpp
    image.cpp
    installer.cpp
    installer_util.cpp
    ramdisk_patcher.cpp
    rom_installer.cpp
    update_binary.cpp
    update_binary_tool.cpp
    utilities.cpp
)

set_source_files_properties(
    miniadbd.cpp
    external/audit/libaudit.h
    external/audit/libaudit.cpp
    external/property_service.cpp
    PROPERTIES
    COMPILE_FLAGS "-Wno-gnu-statement-expression -Wno-zero-length-array"
)

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    # minizip type safety
    add_definitions(-DSTRICTZIPUNZIP)

    # Disable some pugixml features
    add_definitions(
        -DPUGIXML_NO_EXCEPTIONS
        -DPUGIXML_NO_STL
        -DPUGIXML_NO_XPATH
    )

    add_library(
        miniadbd-static
        STATIC
        ${MINIADBD_SOURCES}
    )
    add_executable(
        mbtool
        ${MBTOOL_BASE_SOURCES}
    )
    add_executable(
        mbtool_recovery
        ${MBTOOL_BASE_SOURCES}
        ${MBTOOL_RECOVERY_SOURCES}
    )

    target_compile_definitions(
        mbtool_recovery
        PRIVATE
        -DRECOVERY
    )

    # Ugly, but I don't want to deviate too much from upstream to fix it
    target_compile_options(
        miniadbd-static
        PRIVATE
        -Wno-missing-field-initializers -Wno-pedantic -Wno-unused-parameter
        -Wno-gnu-statement-expression
    )

    set_target_properties(
        miniadbd-static
        mbtool
        mbtool_recovery
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    if(NOT MSVC)
        set_target_properties(
            miniadbd-static
            mbtool
            mbtool_recovery
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
            C_STANDARD 99
            C_STANDARD_REQUIRED 1
        )
    endif()

    set_target_properties(
        mbtool mbtool_recovery
        PROPERTIES
        LINK_FLAGS "-static"
        LINK_SEARCH_START_STATIC ON
    )

    target_link_libraries(
        mbtool
        miniadbd-static
        mbutil-static
        mbsign-static
        mbdevice-static
        mblog-static
        mbbootimg-static
        mbcommon-static
        minizip-static
        ${MBP_JANSSON_LIBRARIES}
        ${MBP_PROCPS_NG_LIBRARIES}
        ${MBP_ZLIB_LIBRARIES}
    )
    target_link_libraries(
        mbtool_recovery
        miniadbd-static
        mbutil-static
        mbsign-static
        mblog-static
        mbdevice-static
        mbbootimg-static
        mbcommon-static
        minizip-static
        ${MBP_JANSSON_LIBRARIES}
        ${MBP_LIBARCHIVE_LIBRARIES}
        ${MBP_LIBLZMA_LIBRARIES}
        ${MBP_LZ4_LIBRARIES}
        ${MBP_LZO_LIBRARIES}
        ${MBP_ZLIB_LIBRARIES}
    )

    install(
        TARGETS mbtool mbtool_recovery
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
