add_definitions(
    -DLIBDBPRAMDISKGALAXY_LIBRARY
)

add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_RAMDISK_GALAXY_SOURCES
    galaxyramdiskpatcher.cpp
    galaxyramdiskplugin.cpp
)

set(DBP_RAMDISK_GALAXY_PRIVATE_HEADERS
    galaxyramdiskpatcher_p.h
)

set(DBP_RAMDISK_GALAXY_HEADERS
    libdbp-ramdisk-galaxy_global.h
    galaxyramdiskpatcher.h
    galaxyramdiskplugin.h
)

add_library(dbp-ramdisk-galaxy SHARED ${DBP_RAMDISK_GALAXY_SOURCES})

set_target_properties(dbp-ramdisk-galaxy PROPERTIES
    OUTPUT_NAME "dbp-ramdisk-galaxy"
)

target_link_libraries(dbp-ramdisk-galaxy
    Qt5::Core
    dbp
    dbp-ramdisk-common
)

# Set rpath for portable build
if (${PORTABLE})
    set_target_properties(dbp-ramdisk-galaxy
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Install library
install(
    TARGETS dbp-ramdisk-galaxy
    LIBRARY DESTINATION ${PLUGINS_INSTALL_DIR} COMPONENT Plugins
    RUNTIME DESTINATION ${PLUGINS_INSTALL_DIR} COMPONENT Plugins
    ARCHIVE DESTINATION ${PLUGINS_INSTALL_DIR} COMPONENT Development
)

# Install headers
install(
    FILES ${DBP_RAMDISK_GALAXY_HEADERS}
    DESTINATION ${HEADERS_INSTALL_DIR}/libdbp-ramdisk-galaxy
    COMPONENT Development
)
