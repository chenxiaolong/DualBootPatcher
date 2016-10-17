find_path(MBP_YAML_CPP_INCLUDES yaml-cpp/yaml.h)
find_library(MBP_YAML_CPP_LIBRARIES NAMES yaml-cpp libyaml-cpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    YAML_CPP DEFAULT_MSG MBP_YAML_CPP_LIBRARIES MBP_YAML_CPP_INCLUDES
)
if(NOT YAML_CPP_FOUND)
    message(FATAL_ERROR "yaml-cpp library was not found")
endif()
