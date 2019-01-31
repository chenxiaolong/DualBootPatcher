project(drm LANGUAGES C ASM)

add_library(
    drm-static
    STATIC
    upstream/xf86drm.c
    upstream/xf86drmHash.c
    upstream/xf86drmRandom.c
    upstream/xf86drmSL.c
    upstream/xf86drmMode.c
)

target_include_directories(
    drm-static
    PUBLIC
    upstream
    upstream/include/drm
)

target_compile_definitions(
    drm-static
    PRIVATE
    -DHAVE_LIBDRM_ATOMIC_PRIMITIVES=1
    -DHAVE_VISIBILITY=1
    -DMAJOR_IN_SYSMACROS=1
)

target_compile_options(
    drm-static
    PRIVATE
    -Wno-assign-enum
    -Wno-bad-function-cast
    -Wno-cast-align
    -Wno-cast-qual
    -Wno-conditional-uninitialized
    -Wno-conversion
    -Wno-deprecated-declarations
    -Wno-documentation
    -Wno-enum-conversion
    -Wno-float-conversion
    -Wno-format
    -Wno-format-nonliteral
    -Wno-format-pedantic
    -Wno-gnu-variable-sized-type-not-at-end
    -Wno-language-extension-token
    -Wno-missing-field-initializers
    -Wno-pointer-arith
    -Wno-shorten-64-to-32
    -Wno-sign-compare
    -Wno-sign-conversion
    -Wno-tautological-compare
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-unreachable-code
)
