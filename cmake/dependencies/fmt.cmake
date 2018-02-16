add_subdirectory(external/fmt)

if(NOT MSVC)
    target_compile_options(
        fmt
        PRIVATE
        -Wno-conversion
        -Wno-format-nonliteral
        -Wno-shadow
        -Wno-sign-conversion
        -Wno-undefined-func-template
        -Wno-unused-parameter
        -Wno-zero-as-null-pointer-constant
    )
endif()
