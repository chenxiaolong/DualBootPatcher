# Scripts
if(MBP_TOP_LEVEL_BUILD)
    # Copy scripts to binary dir so the signing process won't clobber the source
    # directory with .sig files
    file(COPY scripts DESTINATION .)

    add_sign_files_target(
        sign_scripts
        ${CMAKE_CURRENT_BINARY_DIR}/scripts/bb-wrapper.sh
    )

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/scripts/bb-wrapper.sh
              ${CMAKE_CURRENT_BINARY_DIR}/scripts/bb-wrapper.sh.sig
        DESTINATION ${DATA_INSTALL_DIR}/scripts/
        COMPONENT Libraries
    )
endif()
