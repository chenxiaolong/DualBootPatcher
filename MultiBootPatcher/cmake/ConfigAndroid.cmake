# Need ndk-build for building the Android stuff
find_program(NDK_BUILD ndk-build
    HINTS
    "$ENV{ANDROID_NDK_HOME}"
    "$ENV{ANDROID_NDK}"
)

if(NOT NDK_BUILD)
    message(FATAL_ERROR "Could NOT find Android NDK (missing: ndk-build)")
endif()
