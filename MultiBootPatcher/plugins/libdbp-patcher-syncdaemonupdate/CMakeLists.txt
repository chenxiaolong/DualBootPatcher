add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_PATCHER_SYNCDAEMONUPDATE_SOURCES
    syncdaemonupdatepatcher.cpp
    syncdaemonupdateplugin.cpp
)

set(DBP_PATCHER_SYNCDAEMONUPDATE_PRIVATE_HEADERS
    syncdaemonupdatepatcher_p.h
)

set(DBP_PATCHER_SYNCDAEMONUPDATE_HEADERS
    syncdaemonupdatepatcher.h
    syncdaemonupdateplugin.h
)

add_library(dbp-patcher-syncdaemonupdate SHARED ${DBP_PATCHER_SYNCDAEMONUPDATE_SOURCES})

set_target_properties(dbp-patcher-syncdaemonupdate PROPERTIES
    OUTPUT_NAME "dbp-patcher-syncdaemonupdate"
)

target_link_libraries(dbp-patcher-syncdaemonupdate
    Qt5::Core
    dbp
    dbp-ramdisk-common
    ${LibArchive_LIBRARIES}
)

# Set rpath for portable build
if (${PORTABLE})
    set_target_properties(dbp-patcher-syncdaemonupdate
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Install library
install(
    TARGETS dbp-patcher-syncdaemonupdate
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)