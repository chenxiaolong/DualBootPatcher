if(${MBP_BUILD_TARGET} STREQUAL desktop)
    # libmbp tester

    add_executable(
        libmbp_test
        libmbp_test.cpp
    )
    target_link_libraries(
        libmbp_test
        PRIVATE
        mbp-shared
        mbpio-static
        mbdevice-shared
        mblog-shared
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
        PRIVATE
        mbsparse-shared
        mblog-shared
        mbcommon-shared
    )

    if(NOT MSVC)
        set_target_properties(
            desparse
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # binary grep tool

    add_executable(
        bingrep
        bingrep.cpp
    )
    target_link_libraries(
        bingrep
        PRIVATE
        mbcommon-shared
    )

    if(NOT MSVC)
        set_target_properties(
            bingrep
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()
endif()
