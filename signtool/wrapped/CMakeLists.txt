# This sets up signtool as an external project. This is needed because we may
# be cross-compiling, but signtool should always be compiled for the host as it
# needs to run during build-time.

if(MBP_TOP_LEVEL_BUILD)
    include(ExternalProject)

    set(SIGNTOOL_OPTIONS
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DMBP_BUILD_TARGET=signtool
        -DMBP_BUILD_TYPE=${MBP_BUILD_TYPE}
        -DMBP_ENABLE_TESTS=OFF
        -DMBP_INTERNAL_PREBUILTS_DIR=${MBP_PREBUILTS_DIR}
    )

    message(STATUS "Recursive CMake build for signtool:")
    foreach(_option ${SIGNTOOL_OPTIONS})
        message(STATUS "  ${_option}")
    endforeach()

    # Always build system components
    ExternalProject_Add(
        signtool
        SOURCE_DIR ${CMAKE_SOURCE_DIR}
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/result
        CMAKE_ARGS ${SIGNTOOL_OPTIONS}
        BUILD_ALWAYS 1
        INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install/strip
        VERBATIM
    )

    set(SIGNTOOL_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/result/bin/signtool" PARENT_SCOPE)
endif()
