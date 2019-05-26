add_library(
    mbbootui-minzip
    STATIC
    DirUtil.c
    Hash.c
    Inlines.c
    SysUtil.c
    Zip.c
)

target_compile_definitions(
    mbbootui-minzip
    PRIVATE
    -DPLATFORM_SDK_VERSION=${ANDROID_NATIVE_API_LEVEL}
)

target_compile_options(
    mbbootui-minzip
    PRIVATE
    -Wno-language-extension-token
    -Wno-gnu-statement-expression
    -Werror=implicit-function-declaration
    -Werror=unused-function
    -Werror=unused-variable
)

target_link_libraries(
    mbbootui-minzip
    interface.global.CVersion
    SafeIop::SafeIop
    ZLIB::ZLIB
)
