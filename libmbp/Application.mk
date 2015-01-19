APP_ABI := armeabi-v7a arm64-v8a x86 x86_64

ifeq ($(MBP_MINI),true)
APP_PLATFORM := android-21
else
APP_PLATFORM := android-17
endif

APP_STL := gnustl_static
APP_CPPFLAGS := -std=c++11
NDK_TOOLCHAIN_VERSION := 4.9
