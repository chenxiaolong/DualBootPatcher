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
    set(lib_target mbbootimg-${variant})
    string(TOUPPER ${variant} uvariant)

    # Build library
    add_library(
        ${lib_target}
        ${uvariant}
        # Core
        src/entry.cpp
        src/format.cpp
        src/header.cpp
        src/reader.cpp
        src/reader_error.cpp
        src/writer.cpp
        src/writer_error.cpp
        # Formats
        src/format/android_error.cpp
        src/format/android_reader.cpp
        src/format/android_writer.cpp
        src/format/loki.cpp
        src/format/loki_error.cpp
        src/format/loki_reader.cpp
        src/format/loki_writer.cpp
        src/format/mtk_error.cpp
        src/format/mtk_reader.cpp
        src/format/mtk_writer.cpp
        src/format/segment_error.cpp
        src/format/segment_reader.cpp
        src/format/segment_writer.cpp
        src/format/sony_elf_error.cpp
        src/format/sony_elf_reader.cpp
        src/format/sony_elf_writer.cpp
    )

    # Includes
    target_include_directories(${lib_target} PUBLIC include)

    # Only build static library if needed
    if(${variant} STREQUAL static)
        set_target_properties(${lib_target} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbbootimg)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PUBLIC
        mbcommon-${variant}
        PRIVATE
        interface.global.CXXVersion
        interface.mbcommon.library
        interface.mbbootimg.private-headers
        $<$<STREQUAL:${variant},shared>:interface.mbcommon.dynamic-link>
        OpenSSL::Crypto
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
        mbbootimg_tests
        # Helpers
        tests/test_main.cpp
        # Core
        tests/test_entry.cpp
        tests/test_header.cpp
        tests/test_writer.cpp
        # Formats
        tests/format/test_android_reader.cpp
        tests/format/test_android_writer.cpp
        tests/format/test_android_writer_p.cpp
        tests/format/test_bump_reader.cpp
        tests/format/test_bump_writer.cpp
        tests/format/test_loki_reader.cpp
        tests/format/test_loki_writer.cpp
        tests/format/test_mtk_reader.cpp
        tests/format/test_mtk_writer.cpp
        tests/format/test_sony_elf_reader.cpp
        tests/format/test_sony_elf_writer.cpp
    )

    # Link dependencies
    target_link_libraries(
        mbbootimg_tests
        interface.global.CXXVersion
        interface.mbbootimg.private-headers
        mbbootimg-static
        mbcommon-static
        gtest
        gtest_main
        OpenSSL::Crypto
    )

    if(${MBP_BUILD_TARGET} STREQUAL android-system)
        unix_link_executable_statically(mbbootimg_tests)
    endif()

    # Add to ctest
    add_gtest_test(mbbootimg_tests)
endif()

# Interfaces

add_library(interface.mbbootimg.private-headers INTERFACE)

target_compile_definitions(
    interface.mbbootimg.private-headers
    INTERFACE
    -DMBBOOTIMG_BUILD
)
