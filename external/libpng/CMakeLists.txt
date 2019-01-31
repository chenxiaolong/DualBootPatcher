project(png LANGUAGES C ASM)

add_library(
    png-static
    STATIC
    upstream/png.c
    upstream/pngerror.c
    upstream/pngget.c
    upstream/pngmem.c
    upstream/pngpread.c
    upstream/pngread.c
    upstream/pngrio.c
    upstream/pngrtran.c
    upstream/pngrutil.c
    upstream/pngset.c
    upstream/pngtrans.c
    upstream/pngwio.c
    upstream/pngwrite.c
    upstream/pngwtran.c
    upstream/pngwutil.c
)

if("${ANDROID_ABI}" STREQUAL armeabi-v7a OR "${ANDROID_ABI}" STREQUAL arm64-v8a)
    target_sources(
        png-static
        PRIVATE
        upstream/arm/arm_init.c
        upstream/arm/filter_neon.S
        upstream/arm/filter_neon_intrinsics.c
    )
elseif("${ANDROID_ABI}" STREQUAL x86 OR "${ANDROID_ABI}" STREQUAL x86_64)
    target_sources(
        png-static
        PRIVATE
        upstream/contrib/intel/intel_init.c
        upstream/contrib/intel/filter_sse2_intrinsics.c
    )

    target_compile_definitions(
        png-static
        PRIVATE
        -DPNG_INTEL_SSE_OPT=1
    )
endif()

target_include_directories(
    png-static
    PUBLIC
    upstream
)

target_compile_options(
    png-static
    PRIVATE
    -ftrapv
    -Wno-cast-qual
    -Wno-comma
    -Wno-conversion
    -Wno-float-equal
    -Wno-shorten-64-to-32
    -Wno-sign-conversion
    -Wno-unused-parameter
)
