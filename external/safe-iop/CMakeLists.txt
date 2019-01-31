project(safe-iop LANGUAGES C)

add_library(
    safe-iop-static
    STATIC
    upstream/src/safe_iop.c
)

target_include_directories(
    safe-iop-static
    PUBLIC
    upstream/include
)

target_compile_options(
    safe-iop-static
    PRIVATE
    -Wno-documentation
    -Wno-gnu-statement-expression
    -Wno-language-extension-token
)
