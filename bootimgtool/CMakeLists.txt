if(${MBP_BUILD_TARGET} STREQUAL device)
    set(variant static)
else()
    set(variant shared)
endif()

# Build binary
add_executable(bootimgtool bootimgtool.cpp)

if(WIN32)
    target_sources(bootimgtool PRIVATE bootimgtool.rc)
endif()

if(NOT MSVC)
    target_compile_options(
        bootimgtool
        PRIVATE
        -Wno-format-nonliteral
    )
endif()

# Link dependencies
target_link_libraries(
    bootimgtool
    interface.global.CXXVersion
    interface.mbcommon.dynamic-link
    mbbootimg-${variant}
    mbpio-${variant}
    mbcommon-${variant}
    rapidjson
)

if(${variant} STREQUAL static)
    unix_link_executable_statically(bootimgtool)
endif()

# Install binary
install(
    TARGETS bootimgtool
    RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
    COMPONENT Applications
)