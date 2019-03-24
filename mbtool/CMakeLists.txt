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
    src/util/legacy_property_service.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-shorten-64-to-32 -Wno-sign-conversion"
)

set_source_files_properties(
    src/util/property_service.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-gnu-statement-expression -Wno-shorten-64-to-32 -Wno-sign-conversion"
)

set_source_files_properties(
    src/util/sepolpatch.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-keyword-macro"
)

if(${MBP_BUILD_TARGET} STREQUAL device)
    add_library(
        mbtool-util
        STATIC
        src/util/android_api.cpp
        src/util/config.cpp
        src/util/emergency.cpp
        src/util/legacy_property_service.cpp
        src/util/multiboot.cpp
        src/util/property_service.cpp
        src/util/romconfig.cpp
        src/util/roms.cpp
        src/util/sepolpatch.cpp
        src/util/signature.cpp
        src/util/switcher.cpp
        src/util/uevent.cpp
        src/util/wipe.cpp
    )

    add_executable(
        mbtool
        src/boot/audit/libaudit.cpp
        src/boot/auditd.cpp
        src/boot/init.cpp
        src/boot/init/cutils/uevent.cpp
        src/boot/init/devices.cpp
        src/boot/init/uevent_listener.cpp
        src/boot/mount_fstab.cpp
        src/boot/properties.cpp
        src/boot/reboot.cpp
        src/boot/uevent_dump.cpp
        src/boot/uevent_thread.cpp
        src/main.cpp
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
        src/recovery/update_binary.cpp
    )

    add_executable(
        mbtool-shim
        src/main.cpp
        src/shim/init.cpp
    )

    foreach(target mbtool-util mbtool mbtool_recovery mbtool-shim)
        # Includes
        target_include_directories(
            ${target}
            PRIVATE
            include
        )
    endforeach()

    target_compile_definitions(
        mbtool_recovery
        PRIVATE
        -DRECOVERY
    )

    target_compile_definitions(
        mbtool-shim
        PRIVATE
        -DSHIM
    )

    unix_link_executable_statically(mbtool mbtool_recovery mbtool-shim)

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
        sodium
    )

    target_link_libraries(
        mbtool
        PRIVATE
        interface.global.CXXVersion
        mbtool-util
        mbbootimg-static
    )
    target_link_libraries(
        mbtool_recovery
        PRIVATE
        interface.global.CXXVersion
        mbtool-util
        mbbootimg-static
        LibArchive::LibArchive
    )
    target_link_libraries(
        mbtool-shim
        PRIVATE
        interface.global.CXXVersion
        mbtool-util
    )

    install(
        TARGETS mbtool mbtool_recovery mbtool-shim
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
