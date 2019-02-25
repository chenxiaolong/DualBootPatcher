if(${MBP_BUILD_TARGET} STREQUAL device)
    include("${CMAKE_SOURCE_DIR}/cmake/external/GetGitRevisionDescription.cmake")
    git_describe(GIT_VERSION --dirty --always --tags)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/info.prop.in
        ${CMAKE_CURRENT_BINARY_DIR}/info.prop
        @ONLY
    )

    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/info.prop"
        DESTINATION ${DATA_INSTALL_DIR}/
        COMPONENT Libraries
    )
endif()
