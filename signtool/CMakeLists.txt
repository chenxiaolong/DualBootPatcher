if(${MBP_BUILD_TARGET} STREQUAL hosttools)
    add_executable(signtool signtool.cpp)

    target_include_directories(
        signtool
        PRIVATE
        ${MBP_OPENSSL_INCLUDES}
    )

    set_target_properties(
        signtool
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    if(NOT MSVC)
        set_target_properties(
            signtool
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    target_link_libraries(
        signtool
        PRIVATE
        mbsign-shared
        ${MBP_OPENSSL_CRYPTO_LIBRARY}
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
