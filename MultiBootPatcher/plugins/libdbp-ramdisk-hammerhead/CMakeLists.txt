add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_RAMDISK_HAMMERHEAD_SOURCES
    hammerheadramdiskpatcher.cpp
    hammerheadramdiskplugin.cpp
)

set(DBP_RAMDISK_HAMMERHEAD_PRIVATE_HEADERS
    hammerheadramdiskpatcher_p.h
)

set(DBP_RAMDISK_HAMMERHEAD_HEADERS
    hammerheadramdiskpatcher.h
    hammerheadramdiskplugin.h
)

add_library(dbp-ramdisk-hammerhead SHARED ${DBP_RAMDISK_HAMMERHEAD_SOURCES})

set_target_properties(dbp-ramdisk-hammerhead PROPERTIES
    OUTPUT_NAME "dbp-ramdisk-hammerhead"
)

target_link_libraries(dbp-ramdisk-hammerhead
    Qt5::Core
    dbp
    dbp-ramdisk-common
    dbp-ramdisk-qcom
)

# Set rpath for portable build
if (${PORTABLE})
    set_target_properties(dbp-ramdisk-hammerhead
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Install library
install(
    TARGETS dbp-ramdisk-hammerhead
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)
