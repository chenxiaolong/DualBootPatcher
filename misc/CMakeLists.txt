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
        -Wno-pedantic
    )

    set_target_properties(
        file-contexts-tool
        PROPERTIES
        LINK_FLAGS "-pie"
        C_STANDARD 11
        C_STANDARD_REQUIRED 1
        POSITION_INDEPENDENT_CODE 1
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
    )

    set_target_properties(
        fsck-wrapper
        PROPERTIES
        LINK_FLAGS "-static -nostdlib"
        LINK_SEARCH_START_STATIC ON
    )

    install(
        TARGETS fsck-wrapper
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
