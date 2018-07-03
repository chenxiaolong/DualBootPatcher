if(${MBP_BUILD_TARGET} STREQUAL android-system)
    add_executable(odinupdater odinupdater.cpp)
    add_executable(fuse-sparse fuse-sparse.cpp)

    target_link_libraries(
        odinupdater
        PRIVATE
        interface.global.CXXVersion
        mbsparse-static
        mbutil-static
        mbdevice-static
        mbcommon-static
        mblog-static
        LibArchive::LibArchive
    )
    target_link_libraries(
        fuse-sparse
        PRIVATE
        interface.global.CXXVersion
        mbsparse-static
        mbcommon-static
        mblog-static
        Fuse::Fuse
    )

    unix_link_executable_statically(odinupdater fuse-sparse)

    install(
        TARGETS odinupdater fuse-sparse
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
