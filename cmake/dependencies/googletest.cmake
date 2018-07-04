if(MBP_ENABLE_TESTS)
    set(INSTALL_GTEST OFF CACHE BOOL "Enable installation of googletest" FORCE)
    set(INSTALL_GMOCK OFF CACHE BOOL "Enable installation of googlemock" FORCE)

    add_subdirectory(external/googletest)

    if(NOT MSVC)
        foreach(_target gtest gtest_main gmock gmock_main)
            target_compile_options(
                ${_target}
                PRIVATE
                -Wno-conversion
                -Wno-missing-declarations
                # https://github.com/google/googletest/issues/1303
                -Wno-null-dereference
                -Wno-sign-conversion
                -Wno-zero-as-null-pointer-constant
            )

            if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
                target_compile_options(
                    ${_target}
                    PRIVATE
                    -Wno-format-nonliteral
                    -Wno-missing-field-initializers
                    -Wno-missing-noreturn
                    -Wno-missing-prototypes
                    -Wno-missing-variable-declarations
                )
            else()
                target_compile_options(
                    ${_target}
                    PRIVATE
                    -Wno-duplicated-branches
                )
            endif()
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
