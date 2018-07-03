set_source_files_properties(
    src/external/system_properties.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-gnu-statement-expression -Wno-missing-prototypes -Wno-missing-variable-declarations -Wno-shadow -Wno-shorten-64-to-32 -Wno-sign-conversion -Wno-vla-extension -Wno-zero-length-array"
)

set_source_files_properties(
    src/external/system_properties_compat.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-shorten-64-to-32 -Wno-sign-conversion"
)

set(variants)

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    list(APPEND variants static)
endif()
# No shared version

# Build libraries
foreach(variant ${variants})
    set(lib_target mbutil-${variant})
    string(TOUPPER ${variant} uvariant)

    # Build library
    add_library(
        ${lib_target}
        ${uvariant}
        src/archive.cpp
        src/blkid.cpp
        src/chmod.cpp
        src/chown.cpp
        src/cmdline.cpp
        src/command.cpp
        src/copy.cpp
        src/delete.cpp
        src/directory.cpp
        src/file.cpp
        src/fstab.cpp
        src/fts.cpp
        src/hash.cpp
        src/loopdev.cpp
        src/mount.cpp
        src/path.cpp
        src/process.cpp
        src/properties.cpp
        src/reboot.cpp
        src/selinux.cpp
        src/socket.cpp
        src/string.cpp
        src/time.cpp
        src/vibrate.cpp
        src/external/system_properties.cpp
        src/external/system_properties_compat.cpp
        src/result/file_op_result.cpp
    )

    # Includes
    target_include_directories(
        ${lib_target}
        PUBLIC include
        PRIVATE .
    )

    # Only build static library if needed
    if(${variant} STREQUAL static)
        set_target_properties(${lib_target} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbutil)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PUBLIC
        LibSepol::LibSepol
        PRIVATE
        interface.global.CXXVersion
        interface.mbcommon.library
        $<$<STREQUAL:${variant},shared>:interface.mbcommon.dynamic-link>
        mblog-${variant}
        LibArchive::LibArchive
        OpenSSL::Crypto
    )

    # Install shared library
    if(${variant} STREQUAL shared)
        install(
            TARGETS ${lib_target}
            LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
            RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
            #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        )
    endif()
endforeach()
