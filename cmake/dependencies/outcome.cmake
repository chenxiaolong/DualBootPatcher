# outcome is a header only library. We don't use add_subdirectory() because we
# don't need any of the additional things that the CMake scripts provide.
add_library(outcome INTERFACE)

target_include_directories(outcome INTERFACE external/outcome/single-header)
