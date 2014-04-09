# Override these variables using config-override.sh
VERSION="4.1.2"
ANDROID_NDK=/opt/android-ndk
ANDROID_HOME=/opt/android-sdk

# Default for Arch Linux's mingw-w64-* packages
MINGW_PREFIX=i686-w64-mingw32-

# Windows launcher type (C++ or C#)
WIN32_LAUNCHER="C#"

if [ -f "${BUILDDIR}/config-override.sh" ]; then
    . "${BUILDDIR}/config-override.sh"
fi
