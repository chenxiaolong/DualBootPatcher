################################################################################
# Android prebuilts
################################################################################

set(ANDROID_PREBUILTS_VER "20140913")
set(ANDROID_PREBUILTS_BUSYBOX_VER "20140928")
set(ANDROID_PREBUILTS_BOOST_VER "1.53.0")

# (g|s)etf(acl|attr) and GNU patch

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00140/EKv/prebuilts-android-${ANDROID_PREBUILTS_VER}.tar.xz
    ${CMAKE_CURRENT_SOURCE_DIR}/prebuilts-android-${ANDROID_PREBUILTS_VER}.tar.xz
    EXPECTED_MD5 3b740aa9ca5f8f191d0ed6d600c78ffb
    SHOW_PROGRESS
)

# Valgrind

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00140/33H/prebuilts-android-valgrind-${ANDROID_PREBUILTS_VER}.tar
    ${CMAKE_CURRENT_SOURCE_DIR}/prebuilts-android-valgrind-${ANDROID_PREBUILTS_VER}.tar
    EXPECTED_MD5 235011c66908e631c0fa40fac13be9fa
    SHOW_PROGRESS
)

# Busybox

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00142/syi/prebuilds-android-busybox-${ANDROID_PREBUILTS_BUSYBOX_VER}.tar
    ${CMAKE_CURRENT_SOURCE_DIR}/prebuilds-android-busybox-${ANDROID_PREBUILTS_BUSYBOX_VER}.tar
    EXPECTED_MD5 6c418ec985a4d93e4afbe3194bb5534e
    SHOW_PROGRESS
)

# Boost

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00148/Rac/boost-${ANDROID_PREBUILTS_BOOST_VER}_armeabi-v7a.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/boost-${ANDROID_PREBUILTS_BOOST_VER}_armeabi-v7a.tar.bz2
    EXPECTED_MD5 9fea4e9ac32701fd3d3911cfc7ea6d4c
    SHOW_PROGRESS
)

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00148/7ow/boost-${ANDROID_PREBUILTS_BOOST_VER}_arm64-v8a.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/boost-${ANDROID_PREBUILTS_BOOST_VER}_arm64-v8a.tar.bz2
    EXPECTED_MD5 7b22592dd8873803f7a00b43114aa766
    SHOW_PROGRESS
)

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00148/rQa/boost-${ANDROID_PREBUILTS_BOOST_VER}_x86.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/boost-${ANDROID_PREBUILTS_BOOST_VER}_x86.tar.bz2
    EXPECTED_MD5 bf0541af4a2c4d522b76b369415026b1
    SHOW_PROGRESS
)

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00148/0El/boost-${ANDROID_PREBUILTS_BOOST_VER}_x86_64.tar.bz2
    ${CMAKE_CURRENT_SOURCE_DIR}/boost-${ANDROID_PREBUILTS_BOOST_VER}_x86_64.tar.bz2
    EXPECTED_MD5 2361e0b6048a2571226b2e3f3637bc17
    SHOW_PROGRESS
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/android
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${ANDROID_PREBUILTS_TAR}
    COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${ANDROID_PREBUILTS_VALGRIND_TAR}
    COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${ANDROID_PREBUILTS_BUSYBOX_TAR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android
)

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/android/output/
    DESTINATION ${DATA_INSTALL_DIR}/binaries/android/
    COMPONENT Libraries
    FILES_MATCHING
    PATTERN getfattr
    PATTERN setfattr
    PATTERN getfacl
    PATTERN setfacl
    PATTERN patch
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/android/output/armeabi-v7a/busybox-static
    DESTINATION ${DATA_INSTALL_DIR}/binaries/
)


################################################################################
# GNU patch for Windows
################################################################################

execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/cygwin
)

set(CYGWIN_VER "1.7.32-1")
set(CYGWIN_TAR "cygwin-${CYGWIN_VER}.tar.xz")

file(
    DOWNLOAD
    http://mirrors.kernel.org/sourceware/cygwin/x86/release/cygwin/${CYGWIN_TAR}
    ${CMAKE_CURRENT_SOURCE_DIR}/${CYGWIN_TAR}
    EXPECTED_MD5 9882721f47ad8c7d16c5f04e13d2ba9c
    SHOW_PROGRESS
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${CYGWIN_TAR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygwin
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/cygwin1.dll
    DESTINATION ${DATA_INSTALL_DIR}/binaries/windows/
    COMPONENT Libraries
)


set(LIBINTL8_VER "0.18.3.2-2")
set(LIBINTL8_TAR "libintl8-${LIBINTL8_VER}.tar.xz")

file(
    DOWNLOAD
    http://mirrors.kernel.org/sourceware/cygwin/x86/release/gettext/libintl8/${LIBINTL8_TAR}
    ${CMAKE_CURRENT_SOURCE_DIR}/${LIBINTL8_TAR}
    EXPECTED_MD5 48db59bcca1d1437b5facb2997165777
    SHOW_PROGRESS
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${LIBINTL8_TAR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygwin
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/cygintl-8.dll
    DESTINATION ${DATA_INSTALL_DIR}/binaries/windows/
    COMPONENT Libraries
)


set(LIBICONV2_VER "1.14-2")
set(LIBICONV2_TAR "libiconv2-${LIBICONV2_VER}.tar.bz2")

file(
    DOWNLOAD
    http://mirrors.kernel.org/sourceware/cygwin/x86/release/libiconv/libiconv2/${LIBICONV2_TAR}
    ${CMAKE_CURRENT_SOURCE_DIR}/${LIBICONV2_TAR}
    EXPECTED_MD5 a78340002d86f7f586de40887e213944
    SHOW_PROGRESS
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${LIBICONV2_TAR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygwin
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/cygiconv-2.dll
    DESTINATION ${DATA_INSTALL_DIR}/binaries/windows/
    COMPONENT Libraries
)


set(PATCH_VER "2.7.1-1")
set(PATCH_TAR "patch-${PATCH_VER}.tar.bz2")

file(
    DOWNLOAD
    http://mirrors.kernel.org/sourceware/cygwin/x86/release/patch/${PATCH_TAR}
    ${CMAKE_CURRENT_SOURCE_DIR}/${PATCH_TAR}
    EXPECTED_MD5 73428366ed536bd7881754a51d61762d
    SHOW_PROGRESS
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${PATCH_TAR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cygwin
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/cygwin/usr/bin/patch.exe
    DESTINATION ${DATA_INSTALL_DIR}/binaries/windows/
    RENAME hctap.exe
    COMPONENT Libraries
)
