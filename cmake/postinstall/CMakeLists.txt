if(${MBP_BUILD_TARGET} STREQUAL device)
    configure_file(
        SignFilesInstall.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/SignFilesInstall.cmake"
        @ONLY
    )

    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/SignFilesInstall.cmake")
endif()
