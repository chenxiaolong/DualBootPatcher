if(${MBP_BUILD_TARGET} STREQUAL hosttools)
    add_executable(
        mbsystrace-headersgen
        main.cpp
        process.cpp
    )

    target_link_libraries(
        mbsystrace-headersgen
        PRIVATE
        interface.global.CXXVersion
        mbcommon-shared
        pthread
    )

    set_target_properties(
        mbsystrace-headersgen
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN/../lib"
    )

    install(
        TARGETS mbsystrace-headersgen
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
