add_definitions(
    -DLIBDBPRAMDISKCOMMON_LIBRARY
)

add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_RAMDISK_COMMON_SOURCES
    coreramdiskpatcher.cpp
    coreramdiskplugin.cpp
)

set(DBP_RAMDISK_COMMON_PRIVATE_HEADERS
    coreramdiskpatcher_p.h
)

set(DBP_RAMDISK_COMMON_HEADERS
    libdbp-ramdisk-common_global.h
    coreramdiskpatcher.h
    coreramdiskplugin.h
)

add_library(dbp-ramdisk-common SHARED ${DBP_RAMDISK_COMMON_SOURCES})

set_target_properties(dbp-ramdisk-common PROPERTIES
    OUTPUT_NAME "dbp-ramdisk-common"
)

target_link_libraries(dbp-ramdisk-common
    Qt5::Core
    dbp
)

# Install library
install(
    TARGETS dbp-ramdisk-common
    LIBRARY DESTINATION ${PLUGINS_INSTALL_DIR} COMPONENT Plugins
    RUNTIME DESTINATION ${PLUGINS_INSTALL_DIR} COMPONENT Plugins
    ARCHIVE DESTINATION ${PLUGINS_INSTALL_DIR} COMPONENT Development
)

# Install headers
install(
    FILES ${DBP_RAMDISK_COMMON_HEADERS}
    DESTINATION ${HEADERS_INSTALL_DIR}/libdbp-ramdisk-common
    COMPONENT Development
)