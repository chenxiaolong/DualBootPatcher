if(${MBP_BUILD_TARGET} STREQUAL hosttools)
    add_executable(devicesgen devicesgen.cpp)

    target_link_libraries(
        devicesgen
        interface.global.CXXVersion
        mbdevice-shared
        YamlCpp::YamlCpp
        rapidjson
    )

    set_target_properties(
        devicesgen
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN/../lib"
    )

    install(
        TARGETS devicesgen
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
