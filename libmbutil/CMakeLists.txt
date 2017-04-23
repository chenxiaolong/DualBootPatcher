if(NOT ${MBP_BUILD_TARGET} STREQUAL android-system)
    return()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${MBP_LIBARCHIVE_INCLUDES})
include_directories(${MBP_LIBSEPOL_INCLUDES})
include_directories(${MBP_OPENSSL_INCLUDES})

# If enabled, util/properties.cpp will try to dlopen libc.so to read/write
# properties
#add_definitions(-DDYNAMICALLY_LINKED)

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
    external/mntent.c
)

set_source_files_properties(
    src/external/system_properties.cpp
    PROPERTIES
    COMPILE_FLAGS "-Wno-zero-length-array -Wno-gnu-statement-expression -Wno-vla-extension"
)

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    # Build static library

    add_library(mbutil-static STATIC ${MBUTIL_SOURCES})

    set_target_properties(
        mbutil-static
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    if(NOT MSVC)
        set_target_properties(
            mbutil-static
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    target_link_libraries(
        mbutil-static
        ${MBP_LIBSEPOL_LIBRARIES}
        ${MBP_OPENSSL_CRYPTO_LIBRARY}
    )
endif()
