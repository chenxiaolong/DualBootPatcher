add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_RAMDISK_D800_SOURCES
    d800ramdiskpatcher.cpp
    d800ramdiskplugin.cpp
)

set(DBP_RAMDISK_D800_PRIVATE_HEADERS
    d800ramdiskpatcher_p.h
)

set(DBP_RAMDISK_D800_HEADERS
    d800ramdiskpatcher.h
    d800ramdiskplugin.h
)

add_library(dbp-ramdisk-d800 SHARED ${DBP_RAMDISK_D800_SOURCES})

set_target_properties(dbp-ramdisk-d800 PROPERTIES
    OUTPUT_NAME "dbp-ramdisk-d800"
)

target_link_libraries(dbp-ramdisk-d800
    Qt5::Core
    dbp
    dbp-ramdisk-common
    dbp-ramdisk-qcom
)

# Set rpath for portable build
if (${PORTABLE})
    set_target_properties(dbp-ramdisk-d800
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Install library
install(
    TARGETS dbp-ramdisk-d800
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)
