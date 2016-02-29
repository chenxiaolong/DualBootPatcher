# Scripts
if(MBP_TOP_LEVEL_BUILD)
    install(
        DIRECTORY scripts/
        DESTINATION ${DATA_INSTALL_DIR}/scripts
        COMPONENT Libraries
    )
endif()
