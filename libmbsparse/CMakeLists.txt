if(MBP_ENABLE_TESTS)
    include_directories(${GTEST_INCLUDE_DIRS})
endif()

set(MBSPARSE_SOURCES
    src/sparse.cpp
)

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    # Build static library

    add_library(mbsparse-static STATIC ${MBSPARSE_SOURCES})

    target_compile_definitions(
        mbsparse-static
        PRIVATE
        -DMB_LIBRARY
    )

    set_target_properties(
        mbsparse-static
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    if(NOT MSVC)
        set_target_properties(
            mbsparse-static
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()
elseif(${MBP_BUILD_TARGET} STREQUAL desktop)
    # Build shared library

    add_library(mbsparse-shared SHARED ${MBSPARSE_SOURCES})

    target_compile_definitions(
        mbsparse-shared
        PRIVATE
        -DMB_DYNAMIC_LINK
        -DMB_LIBRARY
    )

    set_target_properties(
        mbsparse-shared
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        OUTPUT_NAME mbsparse
    )

    if(NOT MSVC)
        set_target_properties(
            mbsparse-shared
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    target_link_libraries(
        mbsparse-shared
        mblog-shared
    )

    # Install shared library
    install(
        TARGETS mbsparse-shared
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )

    if(MBP_ENABLE_TESTS)
        add_executable(test_sparse tests/test_sparse.cpp)
        target_link_libraries(
            test_sparse
            mbpio-static
            mbsparse-shared
            ${GTEST_BOTH_LIBRARIES}
        )

        if(NOT MSVC)
            set_target_properties(
                test_sparse
                PROPERTIES
                CXX_STANDARD 11
                CXX_STANDARD_REQUIRED 1
                C_STANDARD 99
                C_STANDARD_REQUIRED 1
            )
        endif()

        add_test(NAME test_sparse COMMAND test_sparse)
    endif()
endif()
