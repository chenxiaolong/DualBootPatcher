if(${MBP_BUILD_TARGET} STREQUAL desktop
        OR ${MBP_BUILD_TARGET} STREQUAL android-system)
    set(BOOTIMGTOOL_SOURCES
        bootimgtool.cpp
    )

    add_executable(bootimgtool ${BOOTIMGTOOL_SOURCES})

    if (${MBP_BUILD_TARGET} STREQUAL desktop)
        target_compile_definitions(
            bootimgtool
            PRIVATE
            -DMB_DYNAMIC_LINK
        )

        target_link_libraries(
            bootimgtool
            mbp-shared
            mbpio-static
        )
    else()
        target_link_libraries(
            bootimgtool
            mbp-static
            mbpio-static
            mblog-static
        )

        set_target_properties(
            bootimgtool
            PROPERTIES
            LINK_FLAGS "-static"
            LINK_SEARCH_START_STATIC ON
        )
    endif()

    if(NOT MSVC)
        set_target_properties(
            bootimgtool
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Set rpath for portable build
    if (${MBP_PORTABLE})
        set_target_properties(
            bootimgtool
            PROPERTIES
            BUILD_WITH_INSTALL_RPATH OFF
            INSTALL_RPATH "\$ORIGIN/lib"
        )
    endif()

    install(
        TARGETS bootimgtool
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
