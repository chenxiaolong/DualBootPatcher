if(${MBP_BUILD_TARGET} STREQUAL hosttools)
    add_executable(devicesgen devicesgen.cpp)

    if(NOT MSVC)
        set_target_properties(
            devicesgen
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    target_link_libraries(
        devicesgen
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
