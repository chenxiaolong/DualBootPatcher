add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_RAMDISK_FALCON_SOURCES
    falconramdiskpatcher.cpp
    falconramdiskplugin.cpp
)

set(DBP_RAMDISK_FALCON_PRIVATE_HEADERS
    falconramdiskpatcher_p.h
)

set(DBP_RAMDISK_FALCON_HEADERS
    falconramdiskpatcher.h
    falconramdiskplugin.h
)

add_library(dbp-ramdisk-falcon SHARED ${DBP_RAMDISK_FALCON_SOURCES})

set_target_properties(dbp-ramdisk-falcon PROPERTIES
    OUTPUT_NAME "dbp-ramdisk-falcon"
)

target_link_libraries(dbp-ramdisk-falcon
    Qt5::Core
    dbp
    dbp-ramdisk-common
    dbp-ramdisk-qcom
)

# Set rpath for portable build
if (${PORTABLE})
    set_target_properties(dbp-ramdisk-falcon
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Install library
install(
    TARGETS dbp-ramdisk-falcon
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)
