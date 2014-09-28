add_definitions(
    -DLIBDBPAUTOPATCHERSTANDARD_LIBRARY
)

add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_AUTOPATCHER_STANDARD_SOURCES
    standardpatcher.cpp
    standardplugin.cpp
)

set(DBP_AUTOPATCHER_STANDARD_PRIVATE_HEADERS
    standardpatcher_p.h
)

set(DBP_AUTOPATCHER_STANDARD_HEADERS
    libdbp-autopatcher-standard_global.h
    standardpatcher.h
    standardplugin.h
)

add_library(dbp-autopatcher-standard SHARED ${DBP_AUTOPATCHER_STANDARD_SOURCES})

set_target_properties(dbp-autopatcher-standard PROPERTIES
    OUTPUT_NAME "dbp-autopatcher-standard"
)

target_link_libraries(dbp-autopatcher-standard
    Qt5::Core
    dbp
)

# Install library
install(
    TARGETS dbp-autopatcher-standard
    LIBRARY DESTINATION ${PLUGINS_INSTALL_DIR} COMPONENT Plugins
    RUNTIME DESTINATION ${PLUGINS_INSTALL_DIR} COMPONENT Plugins
    ARCHIVE DESTINATION ${PLUGINS_INSTALL_DIR} COMPONENT Development
)

# Install headers
install(
    FILES ${DBP_AUTOPATCHER_STANDARD_HEADERS}
    DESTINATION ${HEADERS_INSTALL_DIR}/libdbp-autopatcher-standard
    COMPONENT Development
)