if(MBP_ENABLE_TESTS)
    add_subdirectory(external/googletest)

    if(NOT MSVC)
        foreach(_target gtest gtest_main gmock gmock_main)
            target_compile_options(
                ${_target}
                PUBLIC
                -Wno-conversion
                -Wno-duplicated-branches
                -Wno-missing-declarations
                -Wno-sign-conversion
                -Wno-zero-as-null-pointer-constant
            )
        endforeach()
    endif()

    # gmock-port.h requires the gtest headers, but doesn't include them

    get_target_property(
        gtest_include_dirs
        gtest
        INCLUDE_DIRECTORIES
    )
    get_target_property(
        gtest_main_include_dirs
        gtest_main
        INCLUDE_DIRECTORIES
    )

    target_include_directories(
        gmock
        INTERFACE
        ${gtest_include_dirs}
    )
    target_include_directories(
        gmock_main
        INTERFACE
        ${gtest_main_include_dirs}
    )
endif()
