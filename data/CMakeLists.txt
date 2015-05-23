# patch/diff files
install(
    DIRECTORY patches/
    DESTINATION ${DATA_INSTALL_DIR}/patches
    COMPONENT Libraries
)

# patchinfo files
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/patchinfos.gen.xml
    DESTINATION ${DATA_INSTALL_DIR}/
    COMPONENT Libraries
)

# Scripts
install(
    DIRECTORY scripts/
    DESTINATION ${DATA_INSTALL_DIR}/scripts
    COMPONENT Libraries
)
