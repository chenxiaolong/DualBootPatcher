if(${MBP_BUILD_TARGET} STREQUAL desktop)
    # Allow libmbp headers to be found
    include_directories(${CMAKE_SOURCE_DIR})
    include_directories(${CMAKE_SOURCE_DIR}/libmbcommon/include)
    include_directories(${CMAKE_SOURCE_DIR}/libmblog/include)
    include_directories(${CMAKE_SOURCE_DIR}/libmbp/include)
    include_directories(${CMAKE_SOURCE_DIR}/libmbpio/include)

    set(BOOTIMGTOOL_SOURCES
        bootimgtool.cpp
    )

    add_definitions(
        -DMB_DYNAMIC_LINK
    )

    add_executable(bootimgtool ${BOOTIMGTOOL_SOURCES})

    target_link_libraries(
        bootimgtool
        mbp
        mbpio
    )

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
