add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_RAMDISK_HLTE_SOURCES
    hlteramdiskpatcher.cpp
    hlteramdiskplugin.cpp
)

set(DBP_RAMDISK_HLTE_PRIVATE_HEADERS
    hlteramdiskpatcher_p.h
)

set(DBP_RAMDISK_HLTE_HEADERS
    hlteramdiskpatcher.h
    hlteramdiskplugin.h
)

add_library(dbp-ramdisk-hlte SHARED ${DBP_RAMDISK_HLTE_SOURCES})

set_target_properties(dbp-ramdisk-hlte PROPERTIES
    OUTPUT_NAME "dbp-ramdisk-hlte"
)

target_link_libraries(dbp-ramdisk-hlte
    Qt5::Core
    dbp
    dbp-ramdisk-common
    dbp-ramdisk-galaxy
    dbp-ramdisk-qcom
)

# Set rpath for portable build
if (${PORTABLE})
    set_target_properties(dbp-ramdisk-hlte
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Install library
install(
    TARGETS dbp-ramdisk-hlte
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)
