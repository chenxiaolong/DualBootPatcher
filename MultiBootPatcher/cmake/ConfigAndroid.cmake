# Need ndk-build for building the Android stuff
find_program(NDK_BUILD ndk-build
    HINTS
    "$ENV{ANDROID_NDK_HOME}"
    "$ENV{ANDROID_NDK}"
)

if(NOT NDK_BUILD)
    message(FATAL_ERROR "Could NOT find Android NDK (missing: ndk-build)")
endif()

# Pass NDK_DEBUG=1 to ndk-build commands
option(ANDROID_DEBUG "Compile Android targets with debugging symbols" OFF)

set(NDK_ARGS "")

if(ANDROID_DEBUG)
    set(NDK_ARGS "${NDK_ARGS} NDK_DEBUG=1")
endif()

# Install paths
set(DATA_INSTALL_DIR .)

# We only need the data files in the zip on Android

set(CPACK_PACKAGE_NAME "data")
set(CPACK_PACKAGE_VERSION_MAJOR ${DBP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DBP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DBP_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Data files for MultiBootPatcher")
set(CPACK_SOURCE_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
