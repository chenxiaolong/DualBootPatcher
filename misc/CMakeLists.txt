if(${MBP_BUILD_TARGET} STREQUAL android-system
        OR ${MBP_BUILD_TARGET} STREQUAL hosttools)
    add_executable(
        file-contexts-tool
        file-contexts-tool/callbacks.c
        file-contexts-tool/compile.c
        file-contexts-tool/decompile.c
        file-contexts-tool/label_support.c
        file-contexts-tool/main.c
        file-contexts-tool/pcre_shim.c
        file-contexts-tool/regex.c
    )

    target_compile_options(
        file-contexts-tool
        PRIVATE
        -Wno-cast-qual
        -Wno-conversion
        $<$<CXX_COMPILER_ID:GNU>:-Wno-duplicated-branches>
        -Wno-implicit-fallthrough
        -Wno-pedantic
        -Wno-sign-conversion
    )

    target_link_libraries(
        file-contexts-tool
        PRIVATE
        interface.global.CVersion
    )

    if (${MBP_BUILD_TARGET} STREQUAL hosttools)
        target_link_libraries(
            file-contexts-tool
            PRIVATE
            dl
        )
    endif()

    install(
        TARGETS file-contexts-tool
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    # We use assembly for fsck-wrapper because there's no reason that
    #
    #    int main(int argc, int argv) { return 0; }
    #
    # should take a few hundred kilobytes.

    add_executable(fsck-wrapper fsck-wrapper/main.${ANDROID_ABI}.S)

    target_compile_options(
        fsck-wrapper
        PRIVATE
        -nostdlib
        $<$<STREQUAL:${ANDROID_ABI},armeabi-v7a>:-fno-integrated-as>
    )

    set_target_properties(
        fsck-wrapper
        PROPERTIES
        LINK_FLAGS "-nostdlib"
    )

    unix_link_executable_statically(fsck-wrapper)

    install(
        TARGETS fsck-wrapper
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
