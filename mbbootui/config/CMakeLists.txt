add_library(mbbootui-config STATIC config.cpp)

target_include_directories(
    mbbootui-config
    PRIVATE
    ..
)

set_target_properties(
    mbbootui-config
    PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED 1
    POSITION_INDEPENDENT_CODE 1
)
