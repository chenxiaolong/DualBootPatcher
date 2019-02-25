set(variants)

if(${MBP_BUILD_TARGET} STREQUAL device)
    list(APPEND variants static)
elseif(${MBP_BUILD_TARGET} STREQUAL system)
    list(APPEND variants shared)
endif()

foreach(variant ${variants})
    set(bin_target bootimgtool-${variant})

    # Build binary
    add_executable(${bin_target} bootimgtool.cpp)

    if(WIN32)
        target_sources(${bin_target} PRIVATE bootimgtool.rc)
    endif()

    if(NOT MSVC)
        target_compile_options(
            ${bin_target}
            PRIVATE
            -Wno-format-nonliteral
        )
    endif()

    # Set binary name
    set_target_properties(${bin_target} PROPERTIES OUTPUT_NAME bootimgtool)

    # Link dependencies
    target_link_libraries(
        ${bin_target}
        interface.global.CXXVersion
        interface.mbcommon.dynamic-link
        mbbootimg-${variant}
        mbpio-${variant}
        mbcommon-${variant}
        rapidjson
    )

    if(${variant} STREQUAL static)
        unix_link_executable_statically(${bin_target})
    endif()

    # Install binary
    install(
        TARGETS ${bin_target}
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endforeach()
