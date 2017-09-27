if(${MBP_BUILD_TARGET} STREQUAL android-system)
    add_executable(odinupdater odinupdater.cpp)
    add_executable(fuse-sparse fuse-sparse.cpp)

    set_target_properties(
        odinupdater
        fuse-sparse
        PROPERTIES
        LINK_FLAGS "-static"
        LINK_SEARCH_START_STATIC ON
    )

    target_link_libraries(
        odinupdater
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
        interface.global.CXXVersion
        mbsparse-static
        mbcommon-static
        mblog-static
        Fuse::Fuse
    )

    install(
        TARGETS odinupdater fuse-sparse
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
