# Override these variables using config-override.sh
VERSION="4.0.0beta12"
MINGW_PREFIX=i486-mingw32-
ANDROID_NDK=/opt/android-ndk
ANDROID_HOME=/opt/android-sdk

if [ -f "${BUILDDIR}/config-override.sh" ]; then
    . "${BUILDDIR}/config-override.sh"
fi
