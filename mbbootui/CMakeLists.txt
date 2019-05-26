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

# Silence warnings instead of fixing them so we don't deviate from upstream too
# much
add_compile_options(
    -Wno-cast-qual
    -Wno-conditional-uninitialized
    -Wno-conversion
    -Wno-documentation
    -Wno-double-promotion
    -Wno-float-equal
    -Wno-format-nonliteral
    -Wno-header-hygiene
    -Wno-implicit-fallthrough
    -Wno-missing-declarations
    -Wno-missing-prototypes
    -Wno-missing-noreturn
    -Wno-missing-variable-declarations
    -Wno-old-style-cast
    -Wno-pedantic
    -Wno-shadow
    -Wno-shorten-64-to-32
    -Wno-sign-conversion
    -Wno-unreachable-code-return
)

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
    ${CMAKE_SOURCE_DIR}/external/flatbuffers/include
)

unix_link_executable_statically(mbbootui)

target_link_libraries(
    mbbootui
    PRIVATE
    interface.global.CXXVersion
    mbbootui-gui
    mbbootui-config
    mbutil-static
    mbpatcher-static
    mbdevice-static
    mblog-static
    mbcommon-static
    AndroidSystemCore::Cutils
    LibArchive::LibArchive
)

install(
    TARGETS mbbootui
    RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
    COMPONENT Applications
)
