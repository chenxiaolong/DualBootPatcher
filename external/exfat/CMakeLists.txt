project(exfat LANGUAGES C)

add_compile_definitions(
    _FILE_OFFSET_BITS=64
    PACKAGE=\"exfat\"
    VERSION=\"1.3.0\"
)

# libexfat

add_library(
    exfat-static
    STATIC
    upstream/libexfat/cluster.c
    upstream/libexfat/io.c
    upstream/libexfat/log.c
    upstream/libexfat/lookup.c
    upstream/libexfat/mount.c
    upstream/libexfat/node.c
    upstream/libexfat/repair.c
    upstream/libexfat/time.c
    upstream/libexfat/utf.c
    upstream/libexfat/utils.c
)

target_include_directories(
    exfat-static
    PUBLIC
    upstream/libexfat
)

target_compile_options(
    exfat-static
    PRIVATE
    -Wno-address-of-packed-member
    -Wno-conversion
    -Wno-gnu-case-range
    -Wno-shorten-64-to-32
    -Wno-sign-compare
    -Wno-sign-conversion
    -Wno-unused-parameter
)

# libexfat-fsck

add_library(
    exfat-fsck-static
    STATIC
    upstream/fsck/main.c
)

target_include_directories(
    exfat-fsck-static
    PRIVATE
    upstream/fsck
)

target_link_libraries(
    exfat-fsck-static
    PUBLIC
    exfat-static
)

target_compile_options(
    exfat-fsck-static
    PRIVATE
    -Wno-missing-prototypes
    -Wno-missing-variable-declarations
    -Wno-shorten-64-to-32
    -Wno-sign-conversion
)

# libexfat-mkfs

add_library(
    exfat-mkfs-static
    STATIC
    upstream/mkfs/cbm.c
    upstream/mkfs/fat.c
    upstream/mkfs/main.c
    upstream/mkfs/mkexfat.c
    upstream/mkfs/rootdir.c
    upstream/mkfs/uct.c
    upstream/mkfs/uctc.c
    upstream/mkfs/vbr.c
)

target_include_directories(
    exfat-mkfs-static
    PRIVATE
    upstream/mkfs
)

target_compile_options(
    exfat-mkfs-static
    PRIVATE
    -Wno-conversion
    -Wno-shorten-64-to-32
    -Wno-sign-compare
)

target_link_libraries(
    exfat-mkfs-static
    PUBLIC
    exfat-static
)

# libexfat-mount

add_library(
    exfat-mount-static
    STATIC
    upstream/fuse/main.c
)

target_link_libraries(
    exfat-mount-static
    PUBLIC
    exfat-static
)

target_compile_options(
    exfat-mount-static
    PRIVATE
    -Wno-documentation
    -Wno-missing-prototypes
    -Wno-missing-variable-declarations
    -Wno-shorten-64-to-32
    -Wno-sign-conversion
    -Wno-unused-parameter
)

target_link_libraries(
    exfat-mount-static
    PUBLIC
    fuse-static
)

# exfat

add_executable(
    exfat
    upstream/unified.c
)

target_link_libraries(
    exfat
    PRIVATE
    exfat-fsck-static
    exfat-mkfs-static
    exfat-mount-static
)

set_target_properties(
    exfat
    PROPERTIES
    OUTPUT_NAME mount.exfat
)

unix_link_executable_statically(exfat)

install(
    TARGETS exfat
    RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
    COMPONENT Applications
)
