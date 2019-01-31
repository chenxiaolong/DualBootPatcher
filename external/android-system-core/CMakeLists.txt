project(android-system-core LANGUAGES CXX C ASM)

# libcutils

add_library(
    android-cutils-static
    STATIC
    upstream/libcutils/android_get_control_file.cpp
    upstream/libcutils/fs.cpp
    upstream/libcutils/multiuser.cpp
    upstream/libcutils/socket_inaddr_any_server_unix.cpp
    upstream/libcutils/socket_local_client_unix.cpp
    upstream/libcutils/socket_local_server_unix.cpp
    upstream/libcutils/socket_network_client_unix.cpp
    upstream/libcutils/sockets_unix.cpp
    upstream/libcutils/str_parms.cpp

    upstream/libcutils/config_utils.cpp
    upstream/libcutils/fs_config.cpp
    upstream/libcutils/canned_fs_config.cpp
    upstream/libcutils/hashmap.cpp
    upstream/libcutils/iosched_policy.cpp
    upstream/libcutils/load_file.cpp
    upstream/libcutils/native_handle.cpp
    upstream/libcutils/open_memstream.c
    upstream/libcutils/record_stream.cpp
    upstream/libcutils/sched_policy.cpp
    upstream/libcutils/sockets.cpp
    upstream/libcutils/strdup16to8.cpp
    upstream/libcutils/strdup8to16.cpp
    upstream/libcutils/strlcpy.c
    upstream/libcutils/threads.cpp

    upstream/libcutils/ashmem-host.cpp
    upstream/libcutils/trace-host.cpp

    upstream/libcutils/android_reboot.cpp
    upstream/libcutils/ashmem-dev.cpp
    upstream/libcutils/klog.cpp
    upstream/libcutils/partition_utils.cpp
    upstream/libcutils/properties.cpp
    upstream/libcutils/qtaguid.cpp
    upstream/libcutils/trace-dev.cpp
    upstream/libcutils/uevent.cpp
)

if("${ANDROID_ABI}" STREQUAL armeabi-v7a)
    target_sources(
        android-cutils-static
        PRIVATE
        upstream/libcutils/arch-arm/memset32.S
    )
elseif("${ANDROID_ABI}" STREQUAL arm64-v8a)
    target_sources(
        android-cutils-static
        PRIVATE
        upstream/libcutils/arch-arm64/android_memset.S
    )
elseif("${ANDROID_ABI}" STREQUAL x86)
    target_sources(
        android-cutils-static
        PRIVATE
        upstream/libcutils/arch-x86/android_memset16.S
        upstream/libcutils/arch-x86/android_memset32.S
    )
elseif("${ANDROID_ABI}" STREQUAL x86_64)
    target_sources(
        android-cutils-static
        PRIVATE
        upstream/libcutils/arch-x86_64/android_memset16.S
        upstream/libcutils/arch-x86_64/android_memset32.S
    )
endif()

target_include_directories(
    android-cutils-static
    PRIVATE
    upstream/libutils/include
    PUBLIC
    upstream/libcutils/include
)

target_compile_options(
    android-cutils-static
    PRIVATE
    -Wno-c99-extensions
    -Wno-cast-align
    -Wno-cast-qual
    -Wno-conversion
    -Wno-double-promotion
    -Wno-empty-translation-unit
    -Wno-gnu-statement-expression
    -Wno-missing-prototypes
    -Wno-missing-variable-declarations
    -Wno-old-style-cast
    -Wno-shadow
    -Wno-shorten-64-to-32
    -Wno-sign-conversion
    -Wno-unreachable-code-break
    -Wno-variadic-macros
)

target_link_libraries(
    android-cutils-static
    PUBLIC
    android-log-static
)

# libutils

add_library(
    android-utils-static
    STATIC
    upstream/libutils/FileMap.cpp
    upstream/libutils/JenkinsHash.cpp
    upstream/libutils/NativeHandle.cpp
    upstream/libutils/Printer.cpp
    upstream/libutils/PropertyMap.cpp
    #upstream/libutils/RefBase.cpp # Requires libbacktrace
    upstream/libutils/SharedBuffer.cpp
    upstream/libutils/Static.cpp
    upstream/libutils/StopWatch.cpp
    upstream/libutils/String8.cpp
    upstream/libutils/String16.cpp
    upstream/libutils/StrongPointer.cpp
    upstream/libutils/SystemClock.cpp
    upstream/libutils/Threads.cpp
    upstream/libutils/Timers.cpp
    upstream/libutils/Tokenizer.cpp
    upstream/libutils/Unicode.cpp
    upstream/libutils/VectorImpl.cpp
    #upstream/libutils/misc.cpp # Requires libvndksupport

    upstream/libutils/Trace.cpp
)

target_include_directories(
    android-utils-static
    PUBLIC
    upstream/liblog/include
    upstream/libsystem/include
    upstream/libutils/include
)

set_target_properties(
    android-utils-static
    PROPERTIES
    CXX_EXTENSIONS ON
)

target_compile_options(
    android-utils-static
    PRIVATE
    # CMake's CXX_EXTENSIONS doesn't work with NDK Clang
    -std=gnu++11
    -Wno-cast-qual
    -Wno-conversion
    -Wno-documentation
    -Wno-format-nonliteral
    -Wno-format-pedantic
    -Wno-gnu-conditional-omitted-operand
    -Wno-gnu-statement-expression
    -Wno-gnu-zero-variadic-macro-arguments
    -Wno-implicit-fallthrough
    -Wno-language-extension-token
    -Wno-missing-prototypes
    -Wno-missing-variable-declarations
    -Wno-old-style-cast
    -Wno-shadow
    -Wno-shorten-64-to-32
    -Wno-sign-conversion
    -Wno-unreachable-code-return
)

target_link_libraries(
    android-utils-static
    PUBLIC
    android-cutils-static
    safe-iop-static
)

# liblog

add_library(
    android-log-static
    STATIC
    upstream/liblog/config_read.c
    upstream/liblog/config_write.c
    upstream/liblog/local_logger.c
    upstream/liblog/log_event_list.c
    upstream/liblog/log_event_write.c
    upstream/liblog/log_ratelimit.cpp
    upstream/liblog/logger_lock.c
    upstream/liblog/logger_name.c
    upstream/liblog/logger_read.c
    upstream/liblog/logger_write.c
    upstream/liblog/logprint.c
    upstream/liblog/stderr_write.c

    upstream/liblog/event_tag_map.cpp
    upstream/liblog/log_time.cpp
    upstream/liblog/properties.c
    upstream/liblog/pmsg_reader.c
    upstream/liblog/pmsg_writer.c
    upstream/liblog/logd_reader.c
    upstream/liblog/logd_writer.c

)

target_include_directories(
    android-log-static
    PRIVATE
    upstream/libcutils/include
    upstream/libsystem/include
    upstream/libutils/include
    PUBLIC
    upstream/liblog/include
)

target_compile_definitions(
    android-log-static
    PRIVATE
    -DLIBLOG_LOG_TAG=1006
    -DSNET_EVENT_LOG_TAG=1397638484
)

target_compile_options(
    android-log-static
    PRIVATE
    -Wno-assign-enum
    -Wno-cast-align
    -Wno-c99-extensions
    -Wno-cast-qual
    -Wno-conditional-uninitialized
    -Wno-conversion
    -Wno-double-promotion
    -Wno-format-nonliteral
    -Wno-gnu-binary-literal
    -Wno-gnu-empty-initializer
    -Wno-gnu-statement-expression
    -Wno-gnu-zero-variadic-macro-arguments
    -Wno-missing-prototypes
    -Wno-missing-variable-declarations
    -Wno-old-style-cast
    -Wno-shadow
    -Wno-shorten-64-to-32
    -Wno-sign-conversion
    -Wno-strict-prototypes
    -Wno-unused-variable
    -Wno-variadic-macros
    -Wno-vla-extension
)

# libpixelflinger

add_library(
    android-pixelflinger-static
    STATIC
    upstream/libpixelflinger/codeflinger/ARMAssemblerInterface.cpp
    upstream/libpixelflinger/codeflinger/ARMAssemblerProxy.cpp
    upstream/libpixelflinger/codeflinger/CodeCache.cpp
    upstream/libpixelflinger/codeflinger/GGLAssembler.cpp
    upstream/libpixelflinger/codeflinger/load_store.cpp
    upstream/libpixelflinger/codeflinger/blending.cpp
    upstream/libpixelflinger/codeflinger/texturing.cpp
    upstream/libpixelflinger/fixed.cpp # .arm
    upstream/libpixelflinger/picker.cpp # .arm
    upstream/libpixelflinger/pixelflinger.cpp # .arm
    upstream/libpixelflinger/trap.cpp # .arm
    upstream/libpixelflinger/scanline.cpp # .arm
    upstream/libpixelflinger/format.cpp
    upstream/libpixelflinger/clear.cpp
    upstream/libpixelflinger/raster.cpp
    upstream/libpixelflinger/buffer.cpp
)

if("${ANDROID_ABI}" STREQUAL armeabi-v7a)
    target_sources(
        android-pixelflinger-static
        PRIVATE
        upstream/libpixelflinger/codeflinger/ARMAssembler.cpp
        upstream/libpixelflinger/codeflinger/disassem.c
        upstream/libpixelflinger/col32cb16blend.S
        upstream/libpixelflinger/t32cb16blend.S
        upstream/libpixelflinger/col32cb16blend_neon.S
    )

    target_compile_definitions(
        android-pixelflinger-static
        PRIVATE
        -D__ARM_HAVE_NEON
    )
elseif("${ANDROID_ABI}" STREQUAL arm64-v8a)
    target_sources(
        android-pixelflinger-static
        PRIVATE
        upstream/libpixelflinger/codeflinger/Arm64Assembler.cpp
        upstream/libpixelflinger/codeflinger/Arm64Disassembler.cpp
        upstream/libpixelflinger/arch-arm64/col32cb16blend.S
        upstream/libpixelflinger/arch-arm64/t32cb16blend.S
    )
endif()

target_include_directories(
    android-pixelflinger-static
    PUBLIC
    upstream/libpixelflinger
    upstream/libpixelflinger/include
)

target_compile_options(
    android-pixelflinger-static
    PRIVATE
    -fomit-frame-pointer
    -fstrict-aliasing
    -Wno-cast-align
    -Wno-cast-qual
    -Wno-conditional-uninitialized
    -Wno-conversion
    -Wno-double-promotion
    -Wno-expansion-to-defined
    -Wno-format-pedantic
    -Wno-implicit-fallthrough
    -Wno-missing-prototypes
    -Wno-null-pointer-arithmetic
    -Wno-old-style-cast
    -Wno-shadow
    -Wno-sign-conversion
    -Wno-unused-function
)

target_link_libraries(
    android-pixelflinger-static
    PUBLIC
    android-cutils-static
    android-log-static
    android-utils-static
    safe-iop-static
)
