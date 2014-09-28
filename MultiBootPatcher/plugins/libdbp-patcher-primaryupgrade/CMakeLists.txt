add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_PATCHER_PRIMARYUPGRADE_SOURCES
    primaryupgradepatcher.cpp
    primaryupgradeplugin.cpp
)

set(DBP_PATCHER_PRIMARYUPGRADE_PRIVATE_HEADERS
    primaryupgradepatcher_p.h
    primaryupgradeplugin_p.h
)

set(DBP_PATCHER_PRIMARYUPGRADE_HEADERS
    primaryupgradepatcher.h
    primaryupgradeplugin.h
)

add_library(dbp-patcher-primaryupgrade SHARED ${DBP_PATCHER_PRIMARYUPGRADE_SOURCES})

set_target_properties(dbp-patcher-primaryupgrade PROPERTIES
    OUTPUT_NAME "dbp-patcher-primaryupgrade"
)

target_link_libraries(dbp-patcher-primaryupgrade
    Qt5::Core
    dbp
    ${LibArchive_LIBRARIES}
)

# Install library
install(
    TARGETS dbp-patcher-primaryupgrade
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)