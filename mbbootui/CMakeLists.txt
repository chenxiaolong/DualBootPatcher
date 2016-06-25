if(NOT ${MBP_BUILD_TARGET} STREQUAL android-system)
    return()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# For subdirectories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(config)
add_subdirectory(minuitwrp)
add_subdirectory(minzip)
add_subdirectory(gui)

include_directories(${MBP_LIBARCHIVE_INCLUDES})
include_directories(${MBP_LIBCUTILS_INCLUDES})
include_directories(${CMAKE_SOURCE_DIR}/external/flatbuffers/include)

add_executable(
    mbbootui
    daemon_connection.cpp
    data.cpp
    find_file.cpp
    infomanager.cpp
    main.cpp
    twrp-functions.cpp
)

set_target_properties(
    mbbootui
    PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED 1
    POSITION_INDEPENDENT_CODE 1
    LINK_FLAGS "-static"
)

target_link_libraries(
    mbbootui
    mbbootui-gui
    mbbootui-config
    mbutil-static
    mbp-static
    mblog-static
    mbcommon-static
    ${MBP_LIBARCHIVE_LIBRARIES}
)

install(
    TARGETS mbbootui
    RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
    COMPONENT Applications
)
