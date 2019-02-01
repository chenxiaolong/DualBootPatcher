project(fuse LANGUAGES C)

add_library(
    fuse-static
    STATIC
    upstream/android/statvfs.c
    upstream/lib/buffer.c
    upstream/lib/cuse_lowlevel.c
    upstream/lib/fuse.c
    upstream/lib/fuse_kern_chan.c
    upstream/lib/fuse_loop.c
    upstream/lib/fuse_loop_mt.c
    upstream/lib/fuse_lowlevel.c
    upstream/lib/fuse_mt.c
    upstream/lib/fuse_opt.c
    upstream/lib/fuse_session.c
    upstream/lib/fuse_signals.c
    upstream/lib/helper.c
    upstream/lib/mount.c
    upstream/lib/mount_util.c
    upstream/lib/ulockmgr.c
)

target_include_directories(
    fuse-static
    PRIVATE
    upstream/android
    PUBLIC
    upstream/include
)

target_compile_definitions(
    fuse-static
    PRIVATE
    -D_FILE_OFFSET_BITS=64
    -DFUSE_USE_VERSION=26
)

target_compile_options(
    fuse-static
    PRIVATE
    -fno-strict-aliasing
    -Wno-assign-enum
    -Wno-bad-function-cast
    -Wno-cast-align
    -Wno-cast-qual
    -Wno-conversion
    -Wno-documentation
    -Wno-format-nonliteral
    -Wno-format-pedantic
    -Wno-gnu-conditional-omitted-operand
    -Wno-gnu-statement-expression
    -Wno-language-extension-token
    -Wno-pedantic
    -Wno-shorten-64-to-32
    -Wno-sign-conversion
)
