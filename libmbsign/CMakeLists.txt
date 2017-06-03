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
        PRIVATE ${MBP_OPENSSL_INCLUDES}
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

    # Build with -fPIC
    set_target_properties(${lib_target} PROPERTIES POSITION_INDEPENDENT_CODE 1)

    # Target C++11
    if(NOT MSVC)
        set_target_properties(
            ${lib_target}
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbsign)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PUBLIC
        mbcommon-${variant}
        mblog-${variant}
        ${MBP_OPENSSL_CRYPTO_LIBRARY}
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

    # Includes
    target_include_directories(
        mbsign_tests
        PRIVATE
        ${GTEST_INCLUDE_DIRS}
    )

    # Link dependencies
    target_link_libraries(
        mbsign_tests
        mbsign-static
        ${GTEST_BOTH_LIBRARIES}
    )

    # Target C++11
    if(NOT MSVC)
        set_target_properties(
            mbsign_tests
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Add to ctest
    add_test(
        NAME mbsign_tests
        COMMAND mbsign_tests
    )
endif()
