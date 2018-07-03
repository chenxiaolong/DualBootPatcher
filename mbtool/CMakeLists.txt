set_source_files_properties(
    src/boot/audit/libaudit.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-cast-align -Wno-conversion -Wno-gnu-statement-expression -Wno-old-style-cast -Wno-shorten-64-to-32 -Wno-sign-conversion"
)

set_source_files_properties(
    src/boot/auditd.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-gnu-statement-expression -Wno-zero-length-array"
)

set_source_files_properties(
    src/boot/daemon_v3.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-missing-declarations"
)

set_source_files_properties(
    src/boot/packages.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-format-nonliteral"
)

set_source_files_properties(
    src/boot/property_service.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-gnu-statement-expression -Wno-shorten-64-to-32 -Wno-sign-conversion"
)

set_source_files_properties(
    src/util/legacy_property_service.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-shorten-64-to-32 -Wno-sign-conversion"
)

set_source_files_properties(
    src/util/sepolpatch.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-keyword-macro"
)

set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/external/pugixml/src/pugixml.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-double-promotion -Wno-sign-conversion"
)

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    # Generate validcerts.cpp
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/util/validcerts.cpp.in
        ${CMAKE_CURRENT_BINARY_DIR}/gen/validcerts.cpp
    )

    add_library(
        mbtool-util
        STATIC
        src/util/legacy_property_service.cpp
        src/util/multiboot.cpp
        src/util/romconfig.cpp
        src/util/roms.cpp
        src/util/sepolpatch.cpp
        src/util/signature.cpp
        src/util/switcher.cpp
        src/util/wipe.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/gen/validcerts.cpp
    )

    add_executable(
        mbtool
        src/boot/appsync.cpp
        src/boot/appsyncmanager.cpp
        src/boot/audit/libaudit.cpp
        src/boot/auditd.cpp
        src/boot/daemon.cpp
        src/boot/daemon_v3.cpp
        src/boot/emergency.cpp
        src/boot/init.cpp
        src/boot/init/cutils/uevent.cpp
        src/boot/init/devices.cpp
        src/boot/init/uevent_listener.cpp
        src/boot/mount_fstab.cpp
        src/boot/packages.cpp
        src/boot/properties.cpp
        src/boot/property_service.cpp
        src/boot/reboot.cpp
        src/boot/uevent_dump.cpp
        src/boot/uevent_thread.cpp
        src/main.cpp
        ${CMAKE_SOURCE_DIR}/external/pugixml/src/pugixml.cpp
    )

    add_executable(
        mbtool_recovery
        src/main.cpp
        src/recovery/archive_util.cpp
        src/recovery/backup.cpp
        src/recovery/bootimg_util.cpp
        src/recovery/image.cpp
        src/recovery/installer.cpp
        src/recovery/installer_util.cpp
        src/recovery/ramdisk_patcher.cpp
        src/recovery/rom_installer.cpp
        src/recovery/update_binary.cpp
        src/recovery/update_binary_tool.cpp
        src/recovery/utilities.cpp
    )

    foreach(target mbtool-util mbtool mbtool_recovery)
        # Includes
        target_include_directories(
            ${target}
            PRIVATE
            include
            ${CMAKE_SOURCE_DIR}/external/flatbuffers/include
            ${CMAKE_SOURCE_DIR}/external/minizip
            ${CMAKE_SOURCE_DIR}/external/pugixml/src
        )

        # Disable some pugixml features
        target_compile_definitions(
            ${target}
            PRIVATE
            -DPUGIXML_NO_EXCEPTIONS
            -DPUGIXML_NO_STL
            -DPUGIXML_NO_XPATH
        )
    endforeach()

    target_compile_definitions(
        mbtool_recovery
        PRIVATE
        -DRECOVERY
    )

    unix_link_executable_statically(mbtool mbtool_recovery)

    target_link_libraries(
        mbtool-util
        PRIVATE
        interface.global.CXXVersion
        PUBLIC
        mbutil-static
        mbsign-static
        mbdevice-static
        mblog-static
        mbcommon-static
        rapidjson
    )

    target_link_libraries(
        mbtool
        PRIVATE
        interface.global.CXXVersion
        mbtool-util
        mbbootimg-static
        libminizip
        Procps::Procps
    )
    target_link_libraries(
        mbtool_recovery
        PRIVATE
        interface.global.CXXVersion
        mbtool-util
        mbbootimg-static
        libminizip
        LibArchive::LibArchive
        Procps::Procps # TODO
    )

    install(
        TARGETS mbtool mbtool_recovery
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
