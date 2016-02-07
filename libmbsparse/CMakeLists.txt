include(ExternalProject)

if(MBP_ENABLE_TESTS)
    include_directories(${GTEST_INCLUDE_DIRS})
endif()

set(MBSPARSE_SOURCES
    src/sparse.cpp
)

if(${MBP_BUILD_TARGET} STREQUAL desktop)
    # Only needed if building DLL on Windows
    #add_definitions(-DMB_LIBRARY)

    add_library(mbsparse STATIC ${MBSPARSE_SOURCES})

    set_target_properties(
        mbsparse
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    if(NOT MSVC)
        set_target_properties(
            mbsparse
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    if(MBP_ENABLE_TESTS)
        add_executable(test_sparse tests/test_sparse.cpp)
        target_link_libraries(test_sparse
            mbpio
            mbsparse
            mblog
            ${GTEST_BOTH_LIBRARIES}
        )

        if(NOT MSVC)
            set_target_properties(test_sparse PROPERTIES
                CXX_STANDARD 11
                CXX_STANDARD_REQUIRED 1
                C_STANDARD 99
                C_STANDARD_REQUIRED 1
            )
        endif()

        add_test(test_sparse test_sparse)
    endif()
endif()

set(MBSPARSE_SOURCES_STR "")
foreach(_source ${MBSPARSE_SOURCES})
    set(MBSPARSE_SOURCES_STR "${MBSPARSE_SOURCES_STR} ${_source}")
endforeach()

# Generate Android.mk
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.mk
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.include.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.include.mk
    @ONLY
)

ExternalProject_Add(
    mbsparse-android
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out/
    BUILD_COMMAND ${NDK_BUILD}
                  -C <SOURCE_DIR>
                  NDK_PROJECT_PATH=.
                  NDK_APPLICATION_MK=Application.mk
                  NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out
                  NDK_OUT=<TMP_DIR>
                  APP_BUILD_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/Android.mk
                  ${NDK_ARGS}
    INSTALL_COMMAND ""
)

add_dependencies(
    mbsparse-android
    mblog-android
)

# Always rebuild since files may have changed
ExternalProject_Add_Step(
    mbsparse-android
    forcerebuild
    DEPENDEES update
    DEPENDERS configure
    ALWAYS 1
    COMMAND ""
)