# See PyQt5Win32Compile.txt for instructions on compiling PyQt5 for Windows
create_pyqt_windows() {
    local TD="${TARGETDIR}/pythonportable/Lib/site-packages"
    mkdir -p "${TD}"

    mkdir -p "${BUILDDIR}/windowslibraries"
    pushd "${BUILDDIR}/windowslibraries"

    local FILENAME="PyQt5_QtBase5.2.0_Python3.3.3_win32_msvc2010.7z"

    download_md5 \
        http://fs1.d-h.st/download/00102/HZd/${FILENAME} \
        66b3ef676f4d09c1042941ea51b57918 \
        .

    7z x "-o${TD}" ${FILENAME}

    popd

    if [ "x${BUILDTYPE}" != "xci" ]; then
        # Compress executables and libraries
        upx -v --lzma "${TD}"/PyQt5/*.dll
    fi

    # Create qt.conf
    cat >"${TARGETDIR}/pythonportable/qt.conf" <<EOF
[Paths]
Prefix = Lib/site-packages/PyQt5
Binaries = Lib/site-packages/PyQt5
EOF
}

# Unused
create_pyqt_windows_official() {
    local TD="${TARGETDIR}/pythonportable/Lib/site-packages"
    mkdir -p "${TD}"

    mkdir -p "${BUILDDIR}/windowslibraries"
    pushd "${BUILDDIR}/windowslibraries"

    local FILENAME="PyQt5-5.2-gpl-Py3.3-Qt5.2.0-x32.exe"

    download_md5 \
        http://downloads.sourceforge.net/project/pyqt/PyQt5/PyQt-5.2/${FILENAME} \
        453bb7ee20defe37d7c2874f2425545c \
        .

    local TEMPDIR="$(mktemp -d --tmpdir="$(pwd)")"
    pushd "${TEMPDIR}"
    7z x ../${FILENAME}
    cp -r Lib/site-packages/PyQt5/ ${TD}
    cp Lib/site-packages/sip.pyd ${TD}
    cp -r '$_OUTDIR'/* ${TD}/PyQt5/
    popd

    rm -rf "${TEMPDIR}"

    popd

    pushd "${TD}"

    # Remove unused files

    rm "${TD}"/PyQt5/translations/assistant_*
    rm "${TD}"/PyQt5/translations/designer_*
    rm "${TD}"/PyQt5/translations/linguist_*
    rm "${TD}"/PyQt5/translations/qscintilla_*
    rm "${TD}"/PyQt5/translations/qt_help_*

    local DIRS=(
        'doc'
        'examples'
        'include'
        'plugins/accessible'
        'plugins/bearer'
        'plugins/designer'
        'plugins/iconengines'
        'plugins/mediaservice'
        'plugins/playlistformats'
        'plugins/printsupport'
        'plugins/PyQt5'
        'plugins/sqldrivers'
        'qml'
        'qsci'
        'sip'
        'uic'
    )

    local FILES=(
        '_QOpenGLFunctions_ES2.pyd'
        'assistant.exe'
        'designer.exe'
        #'icudt49.dll'
        #'icuin49.dll'
        #'icuuc49.dll'
        'libeay32.dll'
        #'libEGL.dll'
        #'libGLESv2.dll'
        'libmysql.dll'
        'linguist.exe'
        'lrelease.exe'
        'pylupdate5.exe'
        'pyrcc5.exe'
        'QAxContainer.pyd'
        'qcollectiongenerator.exe'
        'qhelpgenerator.exe'
        'qmlscene.exe'
        'Qsci.pyd'
        'qscintilla2.dll'
        'Qt5CLucene.dll'
        'Qt5Designer.dll'
        'Qt5DesignerComponents.dll'
        'Qt5Help.dll'
        'Qt5Multimedia.dll'
        'Qt5MultimediaQuick_p.dll'
        'Qt5MultimediaWidgets.dll'
        'Qt5Network.dll'
        'Qt5OpenGL.dll'
        'Qt5Positioning.dll'
        'Qt5PrintSupport.dll'
        'Qt5Qml.dll'
        'Qt5Quick.dll'
        'Qt5QuickParticles.dll'
        'Qt5Sensors.dll'
        'Qt5SerialPort.dll'
        'Qt5Sql.dll'
        'Qt5Svg.dll'
        'Qt5Test.dll'
        'Qt5WebKit.dll'
        'Qt5WebKitWidgets.dll'
        'Qt5WinExtras.dll'
        'Qt5Xml.dll'
        'Qt5XmlPatterns.dll'
        'QtDesigner.pyd'
        'QtHelp.pyd'
        'QtMultimedia.pyd'
        'QtMultimediaWidgets.pyd'
        'QtNetwork.pyd'
        'QtOpenGL.pyd'
        'QtPositioning.pyd'
        'QtPrintSupport.pyd'
        'QtQml.pyd'
        'QtQuick.pyd'
        'QtSensors.pyd'
        'QtSerialPort.pyd'
        'QtSql.pyd'
        'QtSvg.pyd'
        'QtTest.pyd'
        'QtWebKit.pyd'
        'QtWebKitWidgets.pyd'
        'QtWebProcess.exe'
        'QtWinExtras.pyd'
        'QtXmlPatterns.pyd'
        'ssleay32.dll'
        'sip.exe'
        'xmlpatterns.exe'
    )

    for i in ${DIRS[@]}; do
        rm -r "${TD}"/PyQt5/${i}
    done

    for i in ${FILES[@]}; do
        rm "${TD}"/PyQt5/${i}
    done

    if [ "x${BUILDTYPE}" != "xci" ]; then
        # Compress executables and libraries
        upx -v --lzma "${TD}"/PyQt5/*.dll
    fi

    # Create qt.conf
    cat >"${TARGETDIR}/pythonportable/qt.conf" <<EOF
[Paths]
Prefix = Lib/site-packages/PyQt5
Binaries = Lib/site-packages/PyQt5
EOF

    popd
}
