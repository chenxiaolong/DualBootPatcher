set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(MBDEVICE_SCHEMAS
    schemas/device.json
    schemas/device_list.json
)

make_directory("${generated_dir}")

# This is a terrible hack
# - The hosttools target will build schemas2cpp for the host and use it when
#   building libmbdevice
# - The desktop target will use the hosttools-compiled schemas2cpp binary to
#   support cross-compilation
# - The Android targets will do the same as the desktop target, except that
#   there will be no dependency on hosttools because the binary path is supplied
#   by the parent CMake invocation
if(${MBP_BUILD_TARGET} STREQUAL hosttools)
    set(schemas2cpp_deps schemas2cpp)
    set(schemas2cpp_command $<TARGET_FILE:schemas2cpp>)
elseif(${MBP_BUILD_TARGET} STREQUAL desktop)
    set(schemas2cpp_deps hosttools)
    set(schemas2cpp_command ${SCHEMAS2CPP_COMMAND})
else()
    set(schemas2cpp_deps)
    set(schemas2cpp_command ${SCHEMAS2CPP_COMMAND})
endif()

# Convert schemas to C++ strings
add_custom_command(
    OUTPUT "${generated_dir}/schemas_gen.cpp"
           "${generated_dir}/schemas_gen.h"
    COMMAND "${schemas2cpp_command}"
        ${MBDEVICE_SCHEMAS}
        -o "${generated_dir}/schemas_gen.cpp"
    DEPENDS ${schemas2cpp_deps} ${MBDEVICE_SCHEMAS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating code for JSON schemas"
    VERBATIM
)

set(variants)

if(MBP_TARGET_HAS_BUILDS)
    list(APPEND variants static)
endif()
if(${MBP_BUILD_TARGET} STREQUAL android-app
        OR ${MBP_BUILD_TARGET} STREQUAL desktop
        OR ${MBP_BUILD_TARGET} STREQUAL hosttools)
    list(APPEND variants shared)
endif()

# Build libraries
foreach(variant ${variants})
    set(lib_target mbdevice-${variant})
    string(TOUPPER ${variant} uvariant)

    # Build library
    add_library(
        ${lib_target}
        ${uvariant}
        src/device.cpp
        src/json.cpp
        src/schema.cpp
        src/capi/device.cpp
        src/capi/json.cpp
        ${generated_dir}/schemas_gen.cpp
    )

    # Includes
    target_include_directories(
        ${lib_target}
        PUBLIC
        include
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/generated
    )

    # Only build static library if needed
    if(${variant} STREQUAL static)
        set_target_properties(${lib_target} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbdevice)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PUBLIC
        mbcommon-${variant}
        PRIVATE
        interface.global.CXXVersion
        interface.mbcommon.library
        $<$<STREQUAL:${variant},shared>:interface.mbcommon.dynamic-link>
        rapidjson
    )

    # Install shared library
    if(${variant} STREQUAL shared)
        install(
            TARGETS ${lib_target}
            LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
            RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
            #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        )
    endif()
endforeach()

# Build tests
if(variants AND MBP_ENABLE_TESTS)
    # Build tests
    add_executable(
        mbdevice_tests
        # Helpers
        tests/main.cpp
        # Tests
        tests/test_device.cpp
        tests/test_flags.cpp
        tests/test_json.cpp
    )

    # Link dependencies
    target_link_libraries(
        mbdevice_tests
        interface.global.CXXVersion
        mbdevice-static
        gtest
        gtest_main
    )

    if(${MBP_BUILD_TARGET} STREQUAL android-system)
        unix_link_executable_statically(mbdevice_tests)
    endif()

    # Add to ctest
    add_test(
        NAME mbdevice_tests
        COMMAND mbdevice_tests
    )
endif()

# Build schemas2cpp
if(${MBP_BUILD_TARGET} STREQUAL hosttools)
    add_executable(schemas2cpp schemas2cpp.cpp)

    target_link_libraries(
        schemas2cpp
        PRIVATE
        interface.global.CXXVersion
        mbcommon-shared # Headers only
        rapidjson
    )

    set_target_properties(
        schemas2cpp
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN/../lib"
    )

    install(
        TARGETS schemas2cpp
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
