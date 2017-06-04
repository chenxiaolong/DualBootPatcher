if(${MBP_BUILD_TARGET} STREQUAL hosttools)
    add_executable(devicesgen devicesgen.cpp)

    target_include_directories(
        devicesgen
        PRIVATE
        ${MBP_JANSSON_INCLUDES}
        ${MBP_YAML_CPP_INCLUDES}
    )

    set_target_properties(
        devicesgen
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

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
        ${MBP_JANSSON_LIBRARIES}
        ${MBP_YAML_CPP_LIBRARIES}
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
