if(NOT ${MBP_BUILD_TARGET} STREQUAL android-system
        AND NOT ${MBP_BUILD_TARGET} STREQUAL hosttools)
    return()
endif()

if(MBP_ENABLE_TESTS)
    include_directories(${GTEST_INCLUDE_DIRS})
endif()

include_directories(${MBP_OPENSSL_INCLUDES})

if(MBP_ENABLE_TESTS)
    include_directories(${GTEST_INCLUDE_DIRS})
endif()

set(MBSIGN_SOURCES
    src/mbsign.cpp
)

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    # Build static library

    add_library(mbsign-static STATIC ${MBSIGN_SOURCES})

    target_compile_definitions(
        mbsign-static
        PRIVATE
        -DMB_LIBRARY
    )

    set_target_properties(
        mbsign-static
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    if(NOT MSVC)
        set_target_properties(
            mbsign-static
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()
else()
    # Build shared library

    add_library(mbsign-shared SHARED ${MBSIGN_SOURCES})

    target_compile_definitions(
        mbsign-shared
        PRIVATE
        -DDMB_DYNAMIC_LINK
        -DMB_LIBRARY
    )

    set_target_properties(
        mbsign-shared
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        OUTPUT_NAME mbsign
    )

    target_link_libraries(
        mbsign-shared
        mblog-shared
        ${MBP_OPENSSL_CRYPTO_LIBRARY}
    )

    if(NOT MSVC)
        set_target_properties(
            mbsign-shared
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Install shared library
    install(
        TARGETS mbsign-shared
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )

    if(MBP_ENABLE_TESTS)
        add_executable(test_sign tests/test_sign.cpp)
        target_link_libraries(
            test_sign
            mbsign-shared
            ${GTEST_BOTH_LIBRARIES}
        )

        if(NOT MSVC)
            set_target_properties(
                test_sign
                PROPERTIES
                CXX_STANDARD 11
                CXX_STANDARD_REQUIRED 1
                C_STANDARD 99
                C_STANDARD_REQUIRED 1
            )
        endif()

        add_test(NAME test_sign COMMAND test_sign)
    endif()
endif()
