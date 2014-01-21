create_shortcuts_windows() {
    # EXE file to launch GUI

    local TEMPDIR="$(mktemp -d --tmpdir="$(pwd)")"
    pushd "${TEMPDIR}"

    local ICON="${CURDIR}/Android_GUI/ic_launcher-web.png"

    local SIZES=(256 64 48 40 32 24 20 16)
    local ICONS=()
    for i in ${SIZES[@]}; do
        convert -resize ${i}x${i} ${ICON} output-${i}.ico
        ICONS+=(output-${i}.ico)
    done

    convert ${ICONS[@]} combined.ico

    echo '1 ICON "combined.ico"' > res.rc
    # 24 = RT_MANIFEST, but windres doesn't seem to like it
    echo "1 24 \"${BUILDDIR}/shortcuts/windows-exe.manifest\"" >> res.rc
    ${MINGW_PREFIX}windres res.rc res.o
    popd

    local CXXFLAGS_GUI="-static -Wl,-subsystem,windows"
    local CXXFLAGS_CLI="-static"

    ${MINGW_PREFIX}g++ ${CXXFLAGS_GUI} -DGUI \
        -DSCRIPT='"scripts\\qtmain.py"' \
        ${BUILDDIR}/shortcuts/windows-exe.cpp \
        ${TEMPDIR}/res.o \
        -lshlwapi \
        -o ${TARGETDIR}/PatchFileWindowsGUI.exe

    ${MINGW_PREFIX}g++ ${CXXFLAGS_CLI} \
        -DSCRIPT='"scripts\\patchfile.py"' \
        ${BUILDDIR}/shortcuts/windows-exe.cpp \
        ${TEMPDIR}/res.o \
        -lshlwapi \
        -o ${TARGETDIR}/PatchFileWindows.exe

    ${MINGW_PREFIX}strip \
        ${TARGETDIR}/PatchFileWindowsGUI.exe \
        ${TARGETDIR}/PatchFileWindows.exe

    upx -v --lzma \
        ${TARGETDIR}/PatchFileWindowsGUI.exe \
        ${TARGETDIR}/PatchFileWindows.exe

    rm -rf "${TEMPDIR}"
}

create_shortcuts_linux() {
    gcc -m32 -static \
        -DPYTHON3 -DSCRIPT='"scripts/qtmain.py"' \
        ${BUILDDIR}/shortcuts/linux-bin.c \
        -o ${TARGETDIR}/PatchFileLinuxGUI

    strip ${TARGETDIR}/PatchFileLinuxGUI

    upx -v --lzma ${TARGETDIR}/PatchFileLinuxGUI

    chmod 755 ${TARGETDIR}/PatchFileLinuxGUI

    # No point in creating a launcher for the command line version
}
