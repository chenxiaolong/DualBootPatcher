# Always use bundled minizip
set(USE_AES OFF CACHE BOOL "enables building of aes library" FORCE)
add_subdirectory(external/minizip)

foreach(_target minizip-shared minizip-static)
    if(NOT MSVC)
        target_compile_options(
            ${_target}
            PRIVATE
            -Wno-cast-qual
            -Wno-conversion
            -Wno-missing-declarations
            -Wno-missing-prototypes
            -Wno-sign-conversion
        )

        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            target_compile_options(
                ${_target}
                PRIVATE
                -Wno-format-nonliteral
                -Wno-missing-variable-declarations
                -Wno-shadow
            )
        endif()
    endif()
endforeach()
