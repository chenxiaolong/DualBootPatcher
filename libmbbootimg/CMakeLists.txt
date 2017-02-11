include_directories(${MBP_OPENSSL_INCLUDES})

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

set(MBBOOTIMG_SOURCES
    # Core
    src/entry.cpp
    src/header.cpp
    src/reader.cpp
    src/writer.cpp
    # Formats
    src/format/android_reader.cpp
    src/format/android_writer.cpp
    src/format/bump_reader.cpp
    src/format/bump_writer.cpp
    src/format/loki.cpp
    src/format/loki_reader.cpp
    src/format/loki_writer.cpp
    src/format/mtk_reader.cpp
    src/format/mtk_writer.cpp
    src/format/segment_reader.cpp
    src/format/segment_writer.cpp
    src/format/sony_elf_reader.cpp
    src/format/sony_elf_writer.cpp
)

set(MBBOOTIMG_TESTS_SOURCES
    # Helpers
    tests/test_main.cpp
    # Core
    tests/test_entry.cpp
    tests/test_header.cpp
    tests/test_reader.cpp
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

add_definitions(-DMBBOOTIMG_BUILD)

set(variants)

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    list(APPEND variants static)
elseif(${MBP_BUILD_TARGET} STREQUAL android-app
        OR ${MBP_BUILD_TARGET} STREQUAL desktop
        OR ${MBP_BUILD_TARGET} STREQUAL hosttools)
    list(APPEND variants shared)
endif()

# Build libraries
foreach(variant ${variants})
    set(lib_target mbbootimg-${variant})
    set(obj_target ${lib_target}-obj)

    # Build objects
    add_library(${obj_target} OBJECT ${MBBOOTIMG_SOURCES})

    # Export symbols
    target_compile_definitions(${obj_target} PRIVATE -DMB_LIBRARY)

    # Win32 DLL export
    if(${variant} STREQUAL shared)
        target_compile_definitions(${obj_target} PRIVATE -DMB_DYNAMIC_LINK)
    endif()

    # Build with -fPIC
    set_target_properties(${obj_target} PROPERTIES POSITION_INDEPENDENT_CODE 1)

    # Target C++11
    if(NOT MSVC)
        set_target_properties(
            ${obj_target}
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Build library
    if(${variant} STREQUAL shared)
        add_library(${lib_target} SHARED $<TARGET_OBJECTS:${obj_target}>)
    elseif(${variant} STREQUAL static)
        add_library(${lib_target} STATIC $<TARGET_OBJECTS:${obj_target}>)
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbbootimg)

    # Link dependencies
    if(${variant} STREQUAL shared)
        target_link_libraries(
            ${lib_target}
            mbcommon-${variant}
            ${MBP_OPENSSL_CRYPTO_LIBRARY}
        )
    endif()

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
if(MBP_ENABLE_TESTS)
    foreach(variant ${variants})
        # Link against objects so we don't have to worry about hidden symbols
        set(obj_target mbbootimg-${variant}-obj)

        # Build tests
        add_executable(
            mbbootimg_tests
            ${MBBOOTIMG_TESTS_SOURCES}
            $<TARGET_OBJECTS:${obj_target}>
        )

        # Link dependencies
        target_link_libraries(
            mbbootimg_tests
            mbcommon-${variant}
            ${MBP_OPENSSL_CRYPTO_LIBRARY}
            ${GTEST_BOTH_LIBRARIES}
        )

        # Target C++11
        if(NOT MSVC)
            set_target_properties(
                mbbootimg_tests
                PROPERTIES
                CXX_STANDARD 11
                CXX_STANDARD_REQUIRED 1
            )
        endif()

        # Add to ctest
        add_test(
            NAME mbbootimg_tests
            COMMAND mbbootimg_tests
        )

        # Only need to build the tests once
        break()
    endforeach()
endif()
