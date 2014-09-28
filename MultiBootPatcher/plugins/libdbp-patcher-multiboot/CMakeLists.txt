add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_PATCHER_MULTIBOOT_SOURCES
    multibootpatcher.cpp
    multibootplugin.cpp
)

set(DBP_PATCHER_MULTIBOOT_PRIVATE_HEADERS
    multibootpatcher_p.h
    multibootplugin_p.h
)

set(DBP_PATCHER_MULTIBOOT_HEADERS
    multibootpatcher.h
    multibootplugin.h
)

add_library(dbp-patcher-multiboot SHARED ${DBP_PATCHER_MULTIBOOT_SOURCES})

set_target_properties(dbp-patcher-multiboot PROPERTIES
    OUTPUT_NAME "dbp-patcher-multiboot"
)

target_link_libraries(dbp-patcher-multiboot
    Qt5::Core
    dbp
    ${LibArchive_LIBRARIES}
)

# Install library
install(
    TARGETS dbp-patcher-multiboot
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)