include("${CMAKE_SOURCE_DIR}/cmake/external/GetGitRevisionDescription.cmake")
git_describe(GIT_VERSION --dirty --always --tags)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/gen/version.cpp
    @ONLY
)

# _FILE_OFFSET_BITS is now meaningful in NDK r15c. We can't define it unless we
# target API 24 or newer or else fgetpos() and fsetpos() will be undefined,
# which prevents <cstdio> from being included.
# See: https://github.com/android-ndk/ndk/issues/480
if(NOT ANDROID)
    add_definitions(-D_FILE_OFFSET_BITS=64)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # mb::format() requires non-literal format string
    set_source_files_properties(
        src/string.cpp
        PROPERTIES
        COMPILE_FLAGS
        "-Wno-format-nonliteral"
    )
endif()

set_source_files_properties(
    src/external/iconv.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-conversion -Wno-sign-conversion -Wno-shadow"
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
    set(lib_target mbcommon-${variant})
    string(TOUPPER ${variant} uvariant)

    # Build library
    add_library(
        ${lib_target}
        ${uvariant}
        src/capi/util.cpp
        src/common.cpp
        src/error.cpp
        src/error_code.cpp
        src/file/fd.cpp
        src/file/memory.cpp
        src/file/open_mode.cpp
        src/file/posix.cpp
        src/file/standard.cpp
        src/file.cpp
        src/file_error.cpp
        src/file_util.cpp
        src/locale.cpp
        src/string.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/gen/version.cpp
    )

    if(${MBP_BUILD_TARGET} STREQUAL android-app)
        target_sources(
            ${lib_target}
            PRIVATE
            src/external/iconv.cpp
        )
        target_compile_definitions(
            ${lib_target}
            PRIVATE
            -DUSE_EXTERNAL_ICONV
        )
    elseif(WIN32)
        target_sources(
            ${lib_target}
            PRIVATE
            src/file/win32.cpp
        )
    endif()

    # Includes
    target_include_directories(
        ${lib_target}
        PUBLIC include
    )

    # Only build static library if needed
    if(${variant} STREQUAL static)
        set_target_properties(${lib_target} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()

    if(NOT MSVC)
        # Don't warn on empty format strings
        target_compile_options(${lib_target} PRIVATE -Wno-format-zero-length)
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbcommon)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PUBLIC
        outcome
        PRIVATE
        interface.global.CXXVersion
        interface.mbcommon.library
        interface.mbcommon.private-headers
        $<$<STREQUAL:${variant},shared>:interface.mbcommon.dynamic-link>
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
        mbcommon_tests
        # Helpers
        tests/main.cpp
        # Tests
        tests/file/test_fd.cpp
        tests/file/test_memory.cpp
        tests/file/test_posix.cpp
        tests/test_endian.cpp
        tests/test_error_code.cpp
        tests/test_file_error.cpp
        tests/test_file_util.cpp
        tests/test_flags.cpp
        tests/test_integer.cpp
        tests/test_locale.cpp
        tests/test_string.cpp
    )

    if(WIN32)
        target_sources(
            mbcommon_tests
            PRIVATE
            tests/file/test_win32.cpp
        )
    endif()

    # Don't warn on empty format strings
    if(NOT MSVC)
        target_compile_options(mbcommon_tests PRIVATE -Wno-format-zero-length)
    endif()

    # Link dependencies
    target_link_libraries(
        mbcommon_tests
        interface.global.CXXVersion
        interface.mbcommon.private-headers
        mbcommon-static
        gmock
        gmock_main
    )

    if(${MBP_BUILD_TARGET} STREQUAL android-system)
        unix_link_executable_statically(mbcommon_tests)
    endif()

    # Add to ctest
    add_gtest_test(mbcommon_tests)
endif()

# Interfaces

add_library(interface.mbcommon.library INTERFACE)

target_compile_definitions(
    interface.mbcommon.library
    INTERFACE
    -DMB_LIBRARY
)

add_library(interface.mbcommon.dynamic-link INTERFACE)

target_compile_definitions(
    interface.mbcommon.dynamic-link
    INTERFACE
    -DMB_DYNAMIC_LINK
)

add_library(interface.mbcommon.private-headers INTERFACE)

target_compile_definitions(
    interface.mbcommon.private-headers
    INTERFACE
    -DMBCOMMON_BUILD
)
