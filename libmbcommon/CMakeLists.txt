include("${CMAKE_SOURCE_DIR}/cmake/external/GetGitRevisionDescription.cmake")
git_describe(GIT_VERSION --dirty --always --tags)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/gen/version.cpp
    @ONLY
)

set(MBCOMMON_SOURCES
    src/file/callbacks.cpp
    src/file/fd.cpp
    src/file/filename.cpp
    src/file/memory.cpp
    src/file/posix.cpp
    src/file/vtable.cpp
    src/file.cpp
    src/file_util.cpp
    src/libc/stdio.cpp
    src/libc/string.cpp
    src/locale.cpp
    src/string.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/gen/version.cpp
)

set(MBCOMMON_TESTS_SOURCES
    # Helpers
    tests/main.cpp
    # Tests
    tests/file/test_callbacks.cpp
    tests/file/test_fd.cpp
    tests/file/test_memory.cpp
    tests/file/test_posix.cpp
    tests/test_endian.cpp
    tests/test_file.cpp
    tests/test_file_util.cpp
    tests/test_locale.cpp
    tests/test_string.cpp
)

if(WIN32)
    list(APPEND MBCOMMON_SOURCES src/file/win32.cpp)

    list(APPEND MBCOMMON_TESTS_SOURCES tests/file/test_win32.cpp)
endif()

if(ANDROID)
    list(APPEND MBCOMMON_SOURCES
         src/external/musl/memmem.c)

    if(${ANDROID_PLATFORM_LEVEL} LESS 21)
        list(APPEND MBCOMMON_SOURCES
             src/external/gnulib/getdelim.c
             src/external/gnulib/getline.c)
    endif()
elseif(WIN32)
    list(APPEND MBCOMMON_SOURCES
         src/external/gnulib/getdelim.c
         src/external/gnulib/getline.c
         src/external/musl/memmem.c
    )
endif()

add_definitions(-DMBCOMMON_BUILD -D_FILE_OFFSET_BITS=64)

set(variants)

if(MBP_TARGET_HAS_BUILDS)
    list(APPEND variants static)
endif()
if(${MBP_BUILD_TARGET} STREQUAL android-app
        OR ${MBP_BUILD_TARGET} STREQUAL desktop
        OR ${MBP_BUILD_TARGET} STREQUAL hosttools)
    list(APPEND variants shared)
endif()

# Build libraries
foreach(variant ${variants})
    set(lib_target mbcommon-${variant})

    # Build library
    if(${variant} STREQUAL shared)
        add_library(${lib_target} SHARED ${MBCOMMON_SOURCES})
    elseif(${variant} STREQUAL static)
        add_library(${lib_target} STATIC ${MBCOMMON_SOURCES})
    endif()

    # Includes
    target_include_directories(
        ${lib_target}
        PUBLIC include
        PRIVATE ${MBP_LIBICONV_INCLUDES}
    )

    # Only build static library if needed
    if(${variant} STREQUAL static)
        set_target_properties(${lib_target} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()

    # Export symbols
    target_compile_definitions(${lib_target} PRIVATE -DMB_LIBRARY)

    # Win32 DLL export
    if(${variant} STREQUAL shared)
        target_compile_definitions(${lib_target} PRIVATE -DMB_DYNAMIC_LINK)
    endif()

    # Build with -fPIC
    set_target_properties(${lib_target} PROPERTIES POSITION_INDEPENDENT_CODE 1)

    # Target C++11
    if(NOT MSVC)
        set_target_properties(
            ${lib_target}
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbcommon)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PRIVATE ${MBP_LIBICONV_LIBRARIES}
    )

    # Install shared library
    if(${variant} STREQUAL shared)
        install(
            TARGETS ${lib_target}
            LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
            RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
            #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        )
    endif()
endforeach()

# Build tests
if(variants AND MBP_ENABLE_TESTS)
    # Build tests
    add_executable(
        mbcommon_tests
        ${MBCOMMON_TESTS_SOURCES}
    )

    # Includes
    target_include_directories(
        mbcommon_tests
        PRIVATE
        ${GTEST_INCLUDE_DIRS}
    )

    # Link dependencies
    target_link_libraries(
        mbcommon_tests
        mbcommon-static
        ${GTEST_BOTH_LIBRARIES}
    )

    # Target C++11
    if(NOT MSVC)
        set_target_properties(
            mbcommon_tests
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Add to ctest
    add_test(
        NAME mbcommon_tests
        COMMAND mbcommon_tests
    )
endif()
