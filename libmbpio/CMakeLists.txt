include_directories(external/android-libc)

set(MBP_IO_SOURCES
    src/delete.cpp
    src/directory.cpp
    src/error.cpp
    src/path.cpp
    src/private/utf8.cpp
    src/private/filebase.cpp
    src/private/string.cpp
)

if(WIN32)
    list(
        APPEND
        MBP_IO_SOURCES
        src/win32/delete.cpp
        src/win32/error.cpp
        src/win32/file.cpp
    )
elseif(ANDROID)
    list(
        APPEND
        MBP_IO_SOURCES
        src/android/file.cpp
        src/posix/delete.cpp
    )
else()
    list(
        APPEND
        MBP_IO_SOURCES
        src/posix/delete.cpp
        src/posix/file.cpp
    )
endif()

# Compatibility library for nftw()/ftw() on API 17
if(${MBP_BUILD_TARGET} STREQUAL android-app)
    add_library(
        c_compat
        STATIC
        external/android-libc/ftw.c
        external/android-libc/nftw.c
    )

    set_target_properties(
        c_compat
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )
endif()

if(${MBP_BUILD_TARGET} STREQUAL android-app
        OR ${MBP_BUILD_TARGET} STREQUAL android-system
        OR ${MBP_BUILD_TARGET} STREQUAL desktop)
    add_library(mbpio-static STATIC ${MBP_IO_SOURCES})

    target_compile_definitions(
        mbpio-static
        PRIVATE
        #-DDMB_DYNAMIC_LINK
        -DMB_LIBRARY
    )

    set_target_properties(
        mbpio-static
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    if(NOT MSVC)
        set_target_properties(
            mbpio-static
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()
endif()
