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
