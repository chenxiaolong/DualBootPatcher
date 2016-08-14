include("${CMAKE_SOURCE_DIR}/cmake/external/GetGitRevisionDescription.cmake")
git_describe(GIT_VERSION --dirty --always --tags)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/gen/version.cpp
    @ONLY
)

set(MBCOMMON_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/gen/version.cpp
)

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    # Build static library

    add_library(mbcommon-static STATIC ${MBCOMMON_SOURCES})

    target_compile_definitions(
        mbcommon-static
        PRIVATE
        -DMB_LIBRARY
    )

    set_target_properties(
        mbcommon-static
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    if(NOT MSVC)
        set_target_properties(
            mbcommon-static
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()
elseif(${MBP_BUILD_TARGET} STREQUAL android-app
        OR ${MBP_BUILD_TARGET} STREQUAL desktop
        OR ${MBP_BUILD_TARGET} STREQUAL hosttools)
    # Build shared library

    add_library(mbcommon-shared SHARED ${MBCOMMON_SOURCES})

    target_compile_definitions(
        mbcommon-shared
        PRIVATE
        -DDMB_DYNAMIC_LINK
        -DMB_LIBRARY
    )

    set_target_properties(
        mbcommon-shared
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        OUTPUT_NAME mbcommon
    )

    if(NOT MSVC)
        set_target_properties(
            mbcommon-shared
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Install shared library

    install(
        TARGETS mbcommon-shared
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )
endif()
