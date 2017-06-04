add_library(
    mbbootui-minzip
    STATIC
    DirUtil.c
    Hash.c
    Inlines.c
    SysUtil.c
    Zip.c
)

target_include_directories(
    mbbootui-minzip
    PRIVATE
    ${MBP_SAFE_IOP_INCLUDES}
    ${MBP_ZLIB_INCLUDES}
)

target_compile_definitions(
    mbbootui-minzip
    PRIVATE
    -DPLATFORM_SDK_VERSION=${ANDROID_NATIVE_API_LEVEL}
)

set_target_properties(
    mbbootui-minzip
    PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED 1
    POSITION_INDEPENDENT_CODE 1
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
    ${MBP_SAFE_IOP_LIBRARIES}
    ${MBP_ZLIB_LIBRARIES}
)
