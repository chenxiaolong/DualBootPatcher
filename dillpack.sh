unzip DualBootPatcher-9.3.0-win32.zip
rm DualBootPatcher-9.3.0-win32.zip

pushd DualBootPatcher-9.3.0-win32.zip
dlls=(
    iconv.dll
    libarchive-13.dll
    libbz2-1.dll
    libcrypto-10.dll
    libgcc_s_sjlj-1.dll
    libGLESv2.dll
    libglib-2.0-0.dll
    libharfbuzz-0.dll
    libintl-8.dll
    liblz4.dll
    liblzma-5.dll
    libpcre-1.dll
    libpcre16-0.dll
    libpng16-16.dll
    libstdc++-6.dll
    libwinpthread-1.dll
    libxml2-2.dll
    Qt5Core.dll
    Qt5Gui.dll
    Qt5Widgets.dll
    zlib1.dll
)
for dll in "${dlls[@]}"; do
    cp "${MINGW_ROOT_PATH}/bin/${dll}" .
done

# Qt Windows plugin
mkdir platforms/
cp "${MINGW_ROOT_PATH}"/lib/qt5/plugins/platforms/qwindows.dll platforms/

# Strip exe's and dll's
find -name '*.exe' -o -name '*.dll' -exec strip {} \\+

# Optionally, compress executables and libraries (excluding qwindows plugin)
upx --lzma *.exe *.dll

popd
zip -r DualBootPatcher-9.3.0-win32.zip DualBootPatcher-9.3.0-win32
