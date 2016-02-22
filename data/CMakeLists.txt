# Scripts
if(MBP_TOP_LEVEL_TARGET)
    install(
        DIRECTORY scripts/
        DESTINATION ${DATA_INSTALL_DIR}/scripts
        COMPONENT Libraries
    )
endif()
