# This sets up hosttools as an external project. This is needed because we may
# be cross-compiling, but the host tools should always be compiled for the host
# as they need to run during build-time.

if(MBP_TOP_LEVEL_BUILD)
    include(ExternalProject)

    set(HOSTTOOLS_OPTIONS
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DMBP_BUILD_TARGET=hosttools
        -DMBP_BUILD_TYPE=${MBP_BUILD_TYPE}
        -DMBP_ENABLE_TESTS=OFF
        -DMBP_PREBUILTS_BINARY_DIR=${MBP_PREBUILTS_BINARY_DIR}
    )

    message(STATUS "Recursive CMake build for hosttools:")
    foreach(_option ${HOSTTOOLS_OPTIONS})
        message(STATUS "  ${_option}")
    endforeach()

    # Always build system components
    ExternalProject_Add(
        hosttools
        SOURCE_DIR ${CMAKE_SOURCE_DIR}
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/result
        CMAKE_ARGS ${HOSTTOOLS_OPTIONS}
        BUILD_ALWAYS 1
        INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install/strip
        VERBATIM
    )

    set(hosttools_binaries
        devicesgen
        mbsystrace-headersgen
        schemas2cpp
        signtool
    )

    foreach(binary ${hosttools_binaries})
        string(TOUPPER "${binary}" var)
        string(REGEX REPLACE "[^A-Z0-9]+" "_" var "${var}")
        set(var "${var}_COMMAND")

        set("${var}" "${CMAKE_CURRENT_BINARY_DIR}/result/bin/${binary}" PARENT_SCOPE)
    endforeach()
endif()
