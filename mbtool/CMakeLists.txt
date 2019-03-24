set_source_files_properties(
    src/main/audit/libaudit.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-cast-align -Wno-conversion -Wno-gnu-statement-expression -Wno-old-style-cast -Wno-shorten-64-to-32 -Wno-sign-conversion"
)

set_source_files_properties(
    src/main/auditd.cpp
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
        src/util/kmsg.cpp
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
        src/main/archive_util.cpp
        src/main/audit/libaudit.cpp
        src/main/auditd.cpp
        src/main/backup.cpp
        src/main/bootimg_util.cpp
        src/main/image.cpp
        src/main/boot.cpp
        src/main/init/cutils/uevent.cpp
        src/main/init/devices.cpp
        src/main/init/uevent_listener.cpp
        src/main/installer.cpp
        src/main/installer_util.cpp
        src/main/mount_fstab.cpp
        src/main/properties.cpp
        src/main/ramdisk_patcher.cpp
        src/main/reboot.cpp
        src/main/uevent_dump.cpp
        src/main/uevent_thread.cpp
        src/main/update_binary.cpp
        src/main.cpp
    )

    add_executable(
        mbtool-shim
        src/main.cpp
        src/shim/init.cpp
    )

    foreach(target mbtool-util mbtool mbtool-shim)
        # Includes
        target_include_directories(
            ${target}
            PRIVATE
            include
        )
    endforeach()

    target_compile_definitions(
        mbtool-shim
        PRIVATE
        -DSHIM
    )

    unix_link_executable_statically(mbtool mbtool-shim)

    target_link_libraries(
        mbtool-util
        PRIVATE
        interface.global.CXXVersion
        PUBLIC
        mbutil-static
        mbsign-static
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
        mbtool-shim
        PRIVATE
        interface.global.CXXVersion
        mbtool-util
    )

    install(
        TARGETS mbtool mbtool-shim
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
