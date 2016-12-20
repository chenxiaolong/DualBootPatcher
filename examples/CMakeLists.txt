if(${MBP_BUILD_TARGET} STREQUAL desktop)
    # libmbp tester

    add_executable(
        libmbp_test
        libmbp_test.cpp
    )
    target_link_libraries(
        libmbp_test
        mbp-shared
        mbpio-static
        mbdevice-shared
    )

    if(NOT MSVC)
        set_target_properties(
            libmbp_test
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # desparse tool

    add_executable(
        desparse
        desparse.cpp
    )
    target_link_libraries(
        desparse
        mbpio-static
        mbsparse-shared
        mblog-shared
    )

    if(NOT MSVC)
        set_target_properties(
            desparse
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()
endif()
