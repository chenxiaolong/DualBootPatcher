if(MBP_TOP_LEVEL_BUILD)
    include("${CMAKE_SOURCE_DIR}/cmake/external/GetGitRevisionDescription.cmake")
    git_describe(GIT_VERSION --dirty --always --tags)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/info.prop.in
        ${CMAKE_CURRENT_BINARY_DIR}/info.prop
        @ONLY
    )
endif()

if(NOT ${MBP_BUILD_TARGET} STREQUAL android-system)
    return()
endif()

add_subdirectory(config)
add_subdirectory(minuitwrp)
add_subdirectory(minzip)
add_subdirectory(gui)

add_executable(
    mbbootui
    daemon_connection.cpp
    data.cpp
    find_file.cpp
    infomanager.cpp
    main.cpp
    twrp-functions.cpp
)

target_include_directories(
    mbbootui
    PRIVATE
    .
    ${MBP_LIBARCHIVE_INCLUDES}
    ${MBP_LIBCUTILS_INCLUDES}
    ${CMAKE_SOURCE_DIR}/external/flatbuffers/include
)

set_target_properties(
    mbbootui
    PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED 1
    POSITION_INDEPENDENT_CODE 1
    LINK_FLAGS "-static"
    LINK_SEARCH_START_STATIC ON
)

target_link_libraries(
    mbbootui
    mbbootui-gui
    mbbootui-config
    mbutil-static
    mbp-static
    mbdevice-static
    mblog-static
    mbcommon-static
    ${MBP_JANSSON_LIBRARIES}
    ${MBP_LIBARCHIVE_LIBRARIES}
)

install(
    TARGETS mbbootui
    RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
    COMPONENT Applications
)
