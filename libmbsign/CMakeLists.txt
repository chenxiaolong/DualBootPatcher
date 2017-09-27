set(MBSIGN_SOURCES
    src/mbsign.cpp
)

set(MBSIGN_TESTS_SOURCES
    # Helpers
    tests/main.cpp
    # Tests
    tests/test_sign.cpp
)

set(variants)

if(MBP_TARGET_HAS_BUILDS)
    list(APPEND variants static)
endif()
if(${MBP_BUILD_TARGET} STREQUAL hosttools)
    list(APPEND variants shared)
endif()

# Build libraries
foreach(variant ${variants})
    set(lib_target mbsign-${variant})

    # Build library
    if(${variant} STREQUAL shared)
        add_library(${lib_target} SHARED ${MBSIGN_SOURCES})
    elseif(${variant} STREQUAL static)
        add_library(${lib_target} STATIC ${MBSIGN_SOURCES})
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

    # Export symbols
    target_compile_definitions(${lib_target} PRIVATE -DMB_LIBRARY)

    # Win32 DLL export
    if(${variant} STREQUAL shared)
        target_compile_definitions(${lib_target} PRIVATE -DMB_DYNAMIC_LINK)
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbsign)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PUBLIC
        mbcommon-${variant}
        mblog-${variant}
        OpenSSL::Crypto
        PRIVATE
        interface.global.CXXVersion
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
        mbsign_tests
        ${MBSIGN_TESTS_SOURCES}
    )

    # Link dependencies
    target_link_libraries(
        mbsign_tests
        interface.global.CXXVersion
        mbsign-static
        gtest
        gtest_main
    )

    # Add to ctest
    add_test(
        NAME mbsign_tests
        COMMAND mbsign_tests
    )
endif()
