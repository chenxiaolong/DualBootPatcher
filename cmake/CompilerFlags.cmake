if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

    # Enable warnings
    add_compile_options(-Wall -Wextra -pedantic)

    #add_compile_options(-Werror)

    # Except for "/*" within comment errors (present in doxygen blocks)
    add_compile_options(-Wno-error=comment)

    if(ANDROID)
        add_compile_options(-fno-exceptions -fno-rtti)

        # -Wunknown-attributes - malloc.h
        # -Wzero-length-array - asm-generic/siginfo.h
        add_compile_options(-Wno-unknown-attributes -Wno-zero-length-array)
    endif()

    if(NOT WIN32)
        # Visibility
        add_compile_options(-fvisibility=hidden)

        # Use DT_RPATH instead of DT_RUNPATH because the latter is not transitive
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")

        # Does not work on Windows:
        # https://sourceware.org/bugzilla/show_bug.cgi?id=11539
        add_compile_options(-ffunction-sections -fdata-sections)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
    endif()
endif()

# Remove some warnings caused by the BoringSSL headers on Android
if(ANDROID AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-Wno-gnu-anonymous-struct -Wno-nested-anon-types)
endif()
