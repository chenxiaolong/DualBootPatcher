if(${MBP_BUILD_TARGET} STREQUAL hosttools)
    add_executable(signtool signtool.cpp)

    target_link_libraries(
        signtool
        PRIVATE
        interface.global.CXXVersion
        mbsign-shared
    )

    set_target_properties(
        signtool
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN/../lib"
    )

    install(
        TARGETS signtool
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
