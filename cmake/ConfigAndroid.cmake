# Install paths
set(DATA_INSTALL_DIR .)

# We only need the data files in the zip on Android

set(CPACK_PACKAGE_NAME "data")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Data files for DualBootPatcher")
set(CPACK_SOURCE_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/assets")
