build_android_app() {
    pushd "${ANDROIDGUI}"

    sed "s/@VERSION@/${VERSION}/g" < AndroidManifest.xml.in > AndroidManifest.xml

    if [ "x${1}" = "xrelease" ]; then
        ./gradlew assembleRelease
        mv build/apk/Android_GUI-release-unsigned.apk \
            "${DISTDIR}/${TARGETNAME}-${DEFAULT_DEVICE}-signed.apk"
    else
        ./gradlew assembleDebug
        mv build/apk/Android_GUI-debug-unaligned.apk \
            "${DISTDIR}/${TARGETNAME}-${DEFAULT_DEVICE}-debug.apk"
    fi

    popd
}
