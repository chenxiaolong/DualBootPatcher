set(MBSPARSE_SOURCES
    src/sparse.cpp
)

set(MBSPARSE_TESTS_SOURCES
    # Helpers
    tests/main.cpp
    # Tests
    tests/test_sparse.cpp
)

set(variants)

if(MBP_TARGET_HAS_BUILDS)
    list(APPEND variants static)
endif()
if(${MBP_BUILD_TARGET} STREQUAL desktop)
    list(APPEND variants shared)
endif()

# Build libraries
foreach(variant ${variants})
    set(lib_target mbsparse-${variant})

    # Build library
    if(${variant} STREQUAL shared)
        add_library(${lib_target} SHARED ${MBSPARSE_SOURCES})
    elseif(${variant} STREQUAL static)
        add_library(${lib_target} STATIC ${MBSPARSE_SOURCES})
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
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbsparse)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PUBLIC mbcommon-${variant}
        PRIVATE mblog-${variant}
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
        mbsparse_tests
        ${MBSPARSE_TESTS_SOURCES}
    )

    # Includes
    target_include_directories(
        mbsparse_tests
        PRIVATE
        ${GTEST_INCLUDE_DIRS}
    )

    # Link dependencies
    target_link_libraries(
        mbsparse_tests
        mbsparse-static
        ${GTEST_BOTH_LIBRARIES}
    )

    # Target C++11
    if(NOT MSVC)
        set_target_properties(
            mbsparse_tests
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Add to ctest
    add_test(
        NAME mbsparse_tests
        COMMAND mbsparse_tests
    )
endif()
