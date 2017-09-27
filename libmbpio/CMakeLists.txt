set(MBP_IO_SOURCES
    src/delete.cpp
    src/directory.cpp
    src/error.cpp
    src/path.cpp
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

    target_compile_options(
        c_compat
        PRIVATE
        -Wno-shorten-64-to-32 -Wno-sign-conversion
    )

    target_include_directories(
        c_compat
        PRIVATE external/android-libc
    )

    target_link_libraries(
        c_compat
        PRIVATE
        interface.global.CXXVersion
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

    target_link_libraries(
        mbpio-static
        PRIVATE
        interface.global.CXXVersion
        # Private dependency to pull in the libmbcommon headers. We can't make
        # this public because libmbpio is a bad legacy library that gets
        # included in both shared and static versions of libmbpio.
        mbcommon-static
    )
endif()
