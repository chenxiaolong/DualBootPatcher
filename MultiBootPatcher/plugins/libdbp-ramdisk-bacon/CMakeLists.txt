add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_RAMDISK_BACON_SOURCES
    baconramdiskpatcher.cpp
    baconramdiskplugin.cpp
)

set(DBP_RAMDISK_BACON_PRIVATE_HEADERS
    baconramdiskpatcher_p.h
)

set(DBP_RAMDISK_BACON_HEADERS
    baconramdiskpatcher.h
    baconramdiskplugin.h
)

add_library(dbp-ramdisk-bacon SHARED ${DBP_RAMDISK_BACON_SOURCES})

set_target_properties(dbp-ramdisk-bacon PROPERTIES
    OUTPUT_NAME "dbp-ramdisk-bacon"
)

target_link_libraries(dbp-ramdisk-bacon
    Qt5::Core
    dbp
    dbp-ramdisk-common
    dbp-ramdisk-qcom
)

# Set rpath for portable build
if (${PORTABLE})
    set_target_properties(dbp-ramdisk-bacon
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Install library
install(
    TARGETS dbp-ramdisk-bacon
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)
