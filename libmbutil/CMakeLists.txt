set(MBUTIL_SOURCES
    src/autoclose/dir.cpp
    src/autoclose/file.cpp
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
    src/external/system_properties_compat.c
    external/android_reboot.c
)

set_source_files_properties(
    src/external/system_properties.cpp
    PROPERTIES
    COMPILE_FLAGS "-Wno-zero-length-array -Wno-gnu-statement-expression -Wno-vla-extension"
)

set(variants)

if(MBP_TARGET_HAS_BUILDS)
    list(APPEND variants static)
endif()
# No shared version

# Build libraries
foreach(variant ${variants})
    set(lib_target mbutil-${variant})

    # Build library
    if(${variant} STREQUAL shared)
        add_library(${lib_target} SHARED ${MBUTIL_SOURCES})
    elseif(${variant} STREQUAL static)
        add_library(${lib_target} STATIC ${MBUTIL_SOURCES})
    endif()

    # Includes
    target_include_directories(
        ${lib_target}
        PUBLIC
        include
        PRIVATE
        .
        ${MBP_LIBARCHIVE_INCLUDES}
        ${MBP_LIBSEPOL_INCLUDES}
        ${MBP_OPENSSL_INCLUDES}
    )

    # Only build static library if needed
    if(${variant} STREQUAL static)
        set_target_properties(${lib_target} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()

    # Export symbols
    target_compile_definitions(${lib_target} PRIVATE -DMB_LIBRARY)

    # Win32 DLL export
    if(${variant} STREQUAL shared)
        target_compile_definitions(${lib_target} PRIVATE -DMB_DYNAMIC_LINK)
    endif()

    # Build with -fPIC
    set_target_properties(${lib_target} PROPERTIES POSITION_INDEPENDENT_CODE 1)

    # Target C++11
    if(NOT MSVC)
        set_target_properties(
            ${lib_target}
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbutil)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PRIVATE
        mblog-${variant}
        ${MBP_LIBSEPOL_LIBRARIES}
        ${MBP_OPENSSL_CRYPTO_LIBRARY}
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
