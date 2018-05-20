# Always use bundled minizip
set(USE_BZIP2 OFF CACHE BOOL "Enables building with BZIP2 library" FORCE)
set(USE_LZMA OFF CACHE BOOL "Enables building with LZMA library" FORCE)
set(USE_CRYPT OFF CACHE BOOL "Enables building with PKWARE traditional encryption" FORCE)
set(USE_AES OFF CACHE BOOL "Enables building with AES library" FORCE)

backup_variable(BUILD_SHARED_LIBS)

if(NOT ${MBP_BUILD_TARGET} STREQUAL android-system)
    set(BUILD_SHARED_LIBS TRUE)
endif()

add_subdirectory(external/minizip)

restore_variable(BUILD_SHARED_LIBS)

if(NOT MSVC)
    target_compile_options(
        libminizip
        PRIVATE
        -Wno-conversion
        -Wno-empty-body
        -Wno-pedantic
        -Wno-sign-conversion
    )

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        target_compile_options(
            libminizip
            PRIVATE
            -Wno-unreachable-code
        )
    else()
        target_compile_options(
            libminizip
            PRIVATE
            -Wno-stringop-overflow
        )
    endif()
endif()
