# Configuration file
configure_file(
    patcher.xml.in
    ${CMAKE_CURRENT_BINARY_DIR}/patcher.xml
    @ONLY
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/patcher.xml
    DESTINATION ${DATA_INSTALL_DIR}
    COMPONENT Libraries
)

# init binaries
install(
    DIRECTORY inits/
    DESTINATION ${DATA_INSTALL_DIR}/inits
    COMPONENT Libraries
)

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
install(
    DIRECTORY scripts/
    DESTINATION ${DATA_INSTALL_DIR}/scripts
    COMPONENT Libraries
)
