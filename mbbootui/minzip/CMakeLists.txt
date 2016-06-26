include_directories(${MBP_SAFE_IOP_INCLUDES})
include_directories(${MBP_ZLIB_INCLUDES})

add_definitions(-DPLATFORM_SDK_VERSION=${ANDROID_NATIVE_API_LEVEL})

add_library(
    mbbootui-minzip
    STATIC
    DirUtil.c
    Hash.c
    Inlines.c
    SysUtil.c
    Zip.c
)

set_target_properties(
    mbbootui-minzip
    PROPERTIES
    C_STANDARD 99
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
