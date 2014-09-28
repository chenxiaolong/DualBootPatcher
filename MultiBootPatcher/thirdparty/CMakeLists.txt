################################################################################
# Android prebuilts
################################################################################

set(ANDROID_PREBUILTS_VER "20140913")
set(ANDROID_PREBUILTS_BUSYBOX_VER "20140928")
set(ANDROID_PREBUILTS_TAR "prebuilts-android-${ANDROID_PREBUILTS_VER}.tar.xz")
set(ANDROID_PREBUILTS_VALGRIND_TAR "prebuilts-android-valgrind-${ANDROID_PREBUILTS_VER}.tar")
set(ANDROID_PREBUILTS_BUSYBOX_TAR "prebuilds-android-busybox-${ANDROID_PREBUILTS_BUSYBOX_VER}.tar")

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00140/EKv/${ANDROID_PREBUILTS_TAR}
    ${CMAKE_CURRENT_SOURCE_DIR}/${ANDROID_PREBUILTS_TAR}
    EXPECTED_MD5 3b740aa9ca5f8f191d0ed6d600c78ffb
    SHOW_PROGRESS
)

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00140/33H/${ANDROID_PREBUILTS_VALGRIND_TAR}
    ${CMAKE_CURRENT_SOURCE_DIR}/${ANDROID_PREBUILTS_VALGRIND_TAR}
    EXPECTED_MD5 235011c66908e631c0fa40fac13be9fa
    SHOW_PROGRESS
)

file(
    DOWNLOAD
    http://fs1.d-h.st/download/00142/syi/${ANDROID_PREBUILTS_BUSYBOX_TAR}
    ${CMAKE_CURRENT_SOURCE_DIR}/${ANDROID_PREBUILTS_BUSYBOX_TAR}
    EXPECTED_MD5 6c418ec985a4d93e4afbe3194bb5534e
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
    # Non-armv7 architectures are not supported at the moment
    REGEX "/arm64-v8a(/[^/]+)?$" EXCLUDE
    REGEX "/x86(/[^/]+)?$" EXCLUDE
    REGEX "/x86_64(/[^/]+)?$" EXCLUDE
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

#add_custom_command(
#    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/usr/bin/cygwin1.dll
#    COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_SOURCE_DIR}/${CYGWIN_TAR}
#    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${CYGWIN_TAR}
#    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#    VERBATIM
#)
#add_custom_target(extract-cygwin DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/usr/bin/cygwin1.dll)
#add_dependencies(extract-files extract-cygwin)
#install(
#    FILES ${CMAKE_CURRENT_BINARY_DIR}/usr/bin/cygwin1.dll
#    DESTINATION ${CMAKE_INSTALL_PREFIX}/
#)

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
