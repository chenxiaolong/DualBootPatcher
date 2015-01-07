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

# aroma wrapper (to rebuild, run create-aromawrapper.sh)
set(URL_BASE "https://dl.dropboxusercontent.com/u/486665/MultiBootPatcher/prebuilts")

file(
    DOWNLOAD
    ${URL_BASE}/aromawrapper.zip
    ${CMAKE_CURRENT_BINARY_DIR}/aromawrapper.zip
    EXPECTED_MD5 a0fa41404c71f5bbc6652f09ebe67891
    SHOW_PROGRESS
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/aromawrapper.zip
    DESTINATION ${DATA_INSTALL_DIR}/
    COMPONENT Libraries
)
