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
