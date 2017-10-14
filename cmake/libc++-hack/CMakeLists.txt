add_library(
    c++-hack-static
    STATIC
    src/lib.cpp
)

# Ensure output path is deterministic so it can be added to
# CMAKE_CXX_STANDARD_LIBRARIES

set_target_properties(
    c++-hack-static
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    LIBRARY_OUTPUT_NAME "c++-hack-static.a"
    PREFIX "lib"
)

set(
    MBP_LIBCXX_HACK_PATH
    "${CMAKE_CURRENT_BINARY_DIR}/libc++-hack-static.a"
    PARENT_SCOPE
)
