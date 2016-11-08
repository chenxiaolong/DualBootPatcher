include_directories(${MBP_FUSE_INCLUDES})
include_directories(${MBP_LIBARCHIVE_INCLUDES})

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    add_definitions(-DSTRICTZIPUNZIP)
    add_definitions(-D_FILE_OFFSET_BITS=64)

    add_executable(odinupdater odinupdater.cpp)
    add_executable(fuse-sparse fuse-sparse.cpp)

    set_target_properties(
        odinupdater
        fuse-sparse
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

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
        mblog-static
        ${MBP_LIBARCHIVE_LIBRARIES}
        ${MBP_ZLIB_LIBRARIES}
        ${MBP_JANSSON_LIBRARIES}
    )
    target_link_libraries(
        fuse-sparse
        mbpio-static
        mbsparse-static
        mblog-static
        ${MBP_FUSE_LIBRARIES}
    )

    install(
        TARGETS odinupdater fuse-sparse
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )
endif()
