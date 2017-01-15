# Find the yaml-cpp include directory and library
#
# YAML_CPP_INCLUDE_DIR - Where to find <yaml-cpp/yaml.h>
# YAML_CPP_LIBRARIES   - List of yaml-cpp libraries
# YAML_CPP_FOUND       - True if yaml-cpp found

# Find include directory
find_path(YAML_CPP_INCLUDE_DIR NAMES yaml-cpp/yaml.h)

# Find library
find_library(YAML_CPP_LIBRARY NAMES yaml-cpp libyaml-cpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    YAML_CPP DEFAULT_MSG YAML_CPP_INCLUDE_DIR YAML_CPP_LIBRARY
)

if(YAML_CPP_FOUND)
    set(YAML_CPP_LIBRARIES ${YAML_CPP_LIBRARY})
endif()

mark_as_advanced(YAML_CPP_INCLUDE_DIR YAML_CPP_LIBRARY)
