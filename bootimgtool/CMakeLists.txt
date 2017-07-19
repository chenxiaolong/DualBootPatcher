set(BOOTIMGTOOL_SOURCES
    bootimgtool.cpp
)

if(WIN32)
    list(APPEND BOOTIMGTOOL_SOURCES bootimgtool.rc)
endif()

set(variants)

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    list(APPEND variants static)
elseif(${MBP_BUILD_TARGET} STREQUAL desktop)
    list(APPEND variants shared)
endif()

foreach(variant ${variants})
    set(bin_target bootimgtool-${variant})

    # Build binary
    add_executable(${bin_target} ${BOOTIMGTOOL_SOURCES})

    # Win32 DLL import
    if(${variant} STREQUAL shared)
        target_compile_definitions(${bin_target} PRIVATE -DMB_DYNAMIC_LINK)
    endif()

    # Target C++11
    if(NOT MSVC)
        set_target_properties(
            ${bin_target}
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Set binary name
    set_target_properties(${bin_target} PROPERTIES OUTPUT_NAME bootimgtool)

    # Link dependencies
    if(${variant} STREQUAL shared)
        target_link_libraries(
            ${bin_target}
            mbbootimg-${variant}
            mbpio-static
            mbcommon-${variant}
        )

        # Set rpath for portable build
        if (${MBP_PORTABLE})
            set_target_properties(
                ${bin_target}
                PROPERTIES
                BUILD_WITH_INSTALL_RPATH OFF
                INSTALL_RPATH "\$ORIGIN/lib"
            )
        endif()
    elseif(${variant} STREQUAL static)
        target_link_libraries(
            ${bin_target}
            mbbootimg-${variant}
            mbpio-${variant}
            mbcommon-${variant}
            ${MBP_OPENSSL_CRYPTO_LIBRARY}
        )

        set_target_properties(
            ${bin_target}
            PROPERTIES
            LINK_FLAGS "-static"
            LINK_SEARCH_START_STATIC ON
        )
    endif()

    # Install binary
    install(
        TARGETS ${bin_target}
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endforeach()
