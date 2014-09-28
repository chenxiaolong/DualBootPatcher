add_definitions(
    -DLIBDBPRAMDISKQCOM_LIBRARY
)

add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_RAMDISK_QCOM_SOURCES
    qcomramdiskpatcher.cpp
    qcomramdiskplugin.cpp
)

set(DBP_RAMDISK_QCOM_PRIVATE_HEADERS
    qcomramdiskpatcher_p.h
)

set(DBP_RAMDISK_QCOM_HEADERS
    libdbp-ramdisk-qcom_global.h
    qcomramdiskpatcher.h
    qcomramdiskplugin.h
)

add_library(dbp-ramdisk-qcom SHARED ${DBP_RAMDISK_QCOM_SOURCES})

set_target_properties(dbp-ramdisk-qcom PROPERTIES
    OUTPUT_NAME "dbp-ramdisk-qcom"
)

target_link_libraries(dbp-ramdisk-qcom
    Qt5::Core
    dbp
    dbp-ramdisk-common
)

# Set rpath for portable build
if (${PORTABLE})
    set_target_properties(dbp-ramdisk-qcom
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Install library
install(
    TARGETS dbp-ramdisk-qcom
    LIBRARY DESTINATION ${PLUGINS_INSTALL_DIR} COMPONENT Plugins
    RUNTIME DESTINATION ${PLUGINS_INSTALL_DIR} COMPONENT Plugins
    ARCHIVE DESTINATION ${PLUGINS_INSTALL_DIR} COMPONENT Development
)

# Install headers
install(
    FILES ${DBP_RAMDISK_QCOM_HEADERS}
    DESTINATION ${HEADERS_INSTALL_DIR}/libdbp-ramdisk-qcom
    COMPONENT Development
)