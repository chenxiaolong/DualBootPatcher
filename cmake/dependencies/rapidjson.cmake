# rapidjson is a header only library
add_library(rapidjson INTERFACE)

target_include_directories(rapidjson INTERFACE external/rapidjson/include)

# Enable rapidjson std::string API
target_compile_definitions(rapidjson INTERFACE -DRAPIDJSON_HAS_STDSTRING=1)

if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(rapidjson INTERFACE -Wno-zero-as-null-pointer-constant)
endif()
