set(MBP_IO_SOURCES
    src/delete.cpp
    src/directory.cpp
    src/error.cpp
    src/path.cpp
    src/private/string.cpp
)

if(WIN32)
    list(
        APPEND
        MBP_IO_SOURCES
        src/win32/delete.cpp
        src/win32/error.cpp
    )
elseif(ANDROID)
    list(
        APPEND
        MBP_IO_SOURCES
        src/posix/delete.cpp
    )
else()
    list(
        APPEND
        MBP_IO_SOURCES
        src/posix/delete.cpp
    )
endif()

# Compatibility library for nftw()/ftw() on API 17
if(${MBP_BUILD_TARGET} STREQUAL android-app)
    add_library(
        c_compat
        STATIC
        external/android-libc/ftw.cpp
    )

    target_include_directories(
        c_compat
        PRIVATE external/android-libc
    )

    set_target_properties(
        c_compat
        PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED 1
        POSITION_INDEPENDENT_CODE 1
    )
endif()

if(MBP_TARGET_HAS_BUILDS)
    add_library(mbpio-static STATIC ${MBP_IO_SOURCES})

    # Only build library if needed
    set_target_properties(mbpio-static PROPERTIES EXCLUDE_FROM_ALL 1)

    target_include_directories(
        mbpio-static
        PUBLIC include
    )

    target_compile_definitions(
        mbpio-static
        PRIVATE
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

    target_link_libraries(
        mbpio-static
        # Private dependency to pull in the libmbcommon headers. We can't make
        # this public because libmbpio is a bad legacy library that gets
        # included in both shared and static versions of libmbpio.
        PRIVATE mbcommon-static
    )
endif()
