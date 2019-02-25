include(GNUInstallDirs)

# Install paths

set(BIN_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})

# CPack

set(CPACK_PACKAGE_NAME "DualBootPatcher")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A tool for multibooting Android ROMs")
set(CPACK_GENERATOR "ZIP")
