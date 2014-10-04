add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_RAMDISK_JFLTE_SOURCES
    jflteramdiskpatcher.cpp
    jflteramdiskplugin.cpp
)

set(DBP_RAMDISK_JFLTE_PRIVATE_HEADERS
    jflteramdiskpatcher_p.h
)

set(DBP_RAMDISK_JFLTE_HEADERS
    jflteramdiskpatcher.h
    jflteramdiskplugin.h
)

add_library(dbp-ramdisk-jflte SHARED ${DBP_RAMDISK_JFLTE_SOURCES})

set_target_properties(dbp-ramdisk-jflte PROPERTIES
    OUTPUT_NAME "dbp-ramdisk-jflte"
)

target_link_libraries(dbp-ramdisk-jflte
    Qt5::Core
    dbp
    dbp-ramdisk-common
    dbp-ramdisk-galaxy
    dbp-ramdisk-qcom
)

# Set rpath for portable build
if (${PORTABLE})
    set_target_properties(dbp-ramdisk-jflte
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Install library
install(
    TARGETS dbp-ramdisk-jflte
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)
