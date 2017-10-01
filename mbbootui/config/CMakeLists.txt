add_library(mbbootui-config STATIC config.cpp)

target_include_directories(
    mbbootui-config
    PRIVATE
    ..
)

target_link_libraries(
    mbbootui-config
    PUBLIC
    interface.global.CXXVersion
    mbdevice-static
)
