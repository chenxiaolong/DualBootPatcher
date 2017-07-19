if(MBP_TOP_LEVEL_BUILD)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/create.sh.in
        ${CMAKE_CURRENT_BINARY_DIR}/create.sh
        @ONLY
    )
endif()
