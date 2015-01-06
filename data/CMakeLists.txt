# patch/diff files
install(
    DIRECTORY patches/
    DESTINATION ${DATA_INSTALL_DIR}/patches
    COMPONENT Libraries
)

# patchinfo files
install(
    DIRECTORY patchinfos/
    DESTINATION ${DATA_INSTALL_DIR}/patchinfos
    COMPONENT Libraries
)

# Scripts
#install(
#    DIRECTORY scripts/
#    DESTINATION ${DATA_INSTALL_DIR}/scripts
#    COMPONENT Libraries
#)
