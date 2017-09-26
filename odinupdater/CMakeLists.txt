if(${MBP_BUILD_TARGET} STREQUAL android-system)
    add_executable(odinupdater odinupdater.cpp)
    add_executable(fuse-sparse fuse-sparse.cpp)

    foreach(target odinupdater fuse-sparse)
        target_include_directories(
            ${target}
            PRIVATE
            ${MBP_FUSE_INCLUDES}
            ${MBP_LIBARCHIVE_INCLUDES}
        )
    endforeach()

    if(NOT MSVC)
        set_target_properties(
            odinupdater
            fuse-sparse
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    set_target_properties(
        odinupdater
        fuse-sparse
        PROPERTIES
        LINK_FLAGS "-static"
        LINK_SEARCH_START_STATIC ON
    )

    target_link_libraries(
        odinupdater
        mbsparse-static
        mbutil-static
        mbdevice-static
        mbcommon-static
        mblog-static
        ${MBP_LIBARCHIVE_LIBRARIES}
        ${MBP_ZLIB_LIBRARIES}
    )
    target_link_libraries(
        fuse-sparse
        mbsparse-static
        mbcommon-static
        mblog-static
        ${MBP_FUSE_LIBRARIES}
    )

    install(
        TARGETS odinupdater fuse-sparse
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
