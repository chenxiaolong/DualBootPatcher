# Always use bundled minizip
set(USE_BZIP2 OFF CACHE BOOL "Enables building with BZIP2 library" FORCE)
set(USE_LZMA OFF CACHE BOOL "Enables building with LZMA library" FORCE)
set(USE_CRYPT OFF CACHE BOOL "Enables building with PKWARE traditional encryption" FORCE)
set(USE_AES OFF CACHE BOOL "Enables building with AES library" FORCE)
add_subdirectory(external/minizip)

foreach(_target libminizip-shared libminizip-static)
    if(NOT MSVC)
        target_compile_options(
            ${_target}
            PRIVATE
            -Wno-cast-qual
            -Wno-conversion
            -Wno-empty-body
            -Wno-missing-declarations
            -Wno-missing-field-initializers
            -Wno-missing-prototypes
            -Wno-pedantic
            -Wno-sign-compare
            -Wno-sign-conversion
            -Wno-unused-parameter
            -Wno-unused-variable
        )

        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            target_compile_options(
                ${_target}
                PRIVATE
                -Wno-missing-variable-declarations
            )
        else()
            target_compile_options(
                ${_target}
                PRIVATE
                -Wno-unused-but-set-variable
            )
        endif()
    endif()
endforeach()
