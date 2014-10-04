add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_RAMDISK_KLTE_SOURCES
    klteramdiskpatcher.cpp
    klteramdiskplugin.cpp
)

set(DBP_RAMDISK_KLTE_PRIVATE_HEADERS
    klteramdiskpatcher_p.h
)

set(DBP_RAMDISK_KLTE_HEADERS
    klteramdiskpatcher.h
    klteramdiskplugin.h
)

add_library(dbp-ramdisk-klte SHARED ${DBP_RAMDISK_KLTE_SOURCES})

set_target_properties(dbp-ramdisk-klte PROPERTIES
    OUTPUT_NAME "dbp-ramdisk-klte"
)

target_link_libraries(dbp-ramdisk-klte
    Qt5::Core
    dbp
    dbp-ramdisk-common
    dbp-ramdisk-galaxy
    dbp-ramdisk-qcom
)

# Set rpath for portable build
if (${PORTABLE})
    set_target_properties(dbp-ramdisk-klte
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Install library
install(
    TARGETS dbp-ramdisk-klte
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)
