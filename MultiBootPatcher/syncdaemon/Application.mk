APP_BUILD_SCRIPT := Android.mk

# The patcher only supports ARM devices at the moment
APP_ABI := all
#APP_ABI := armeabi-v7a

APP_PLATFORM := android-17

# GNU libstdc++ is needed for C++11's <thread>
APP_STL := gnustl_static
APP_CPPFLAGS := -std=c++11
NDK_TOOLCHAIN_VERSION := 4.8
