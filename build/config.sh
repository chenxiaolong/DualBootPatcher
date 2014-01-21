# Override these variables using config-override.sh
VERSION="4.0.0beta12"
ANDROID_NDK=/opt/android-ndk
ANDROID_HOME=/opt/android-sdk

# Default for Arch Linux's mingw-w64-* packages
MINGW_PREFIX=i686-w64-mingw32-


if [ -f "${BUILDDIR}/config-override.sh" ]; then
    . "${BUILDDIR}/config-override.sh"
fi
