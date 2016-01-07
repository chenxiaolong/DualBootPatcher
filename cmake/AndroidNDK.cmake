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
option(MBP_ANDROID_DEBUG "Compile Android targets with debugging symbols" OFF)

set(NDK_ARGS "")

if(MBP_ANDROID_DEBUG)
    list(APPEND NDK_ARGS "NDK_DEBUG=1")
endif()

# -j option to pass to ndk-build
set(MBP_ANDROID_NDK_PARALLEL_PROCESSES "${PROCESSOR_COUNT}"
    CACHE STRING "-j<COUNT> parameter to pass to ndk-build")

#set(NDK_ARGS "${NDK_ARGS} -j${MBP_ANDROID_NDK_PARALLEL_PROCESSES}")
list(APPEND NDK_ARGS "-j${MBP_ANDROID_NDK_PARALLEL_PROCESSES}")