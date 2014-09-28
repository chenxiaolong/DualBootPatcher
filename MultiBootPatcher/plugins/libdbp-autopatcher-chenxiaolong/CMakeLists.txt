add_definitions(
    -DLIBDBPAUTOPATCHERCHENXIAOLONG_LIBRARY
)

add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_AUTOPATCHER_CHENXIAOLONG_SOURCES
    noobdevpatcher.cpp
    noobdevplugin.cpp
)

set(DBP_AUTOPATCHER_CHENXIAOLONG_PRIVATE_HEADERS
    noobdevpatcher_p.h
)

set(DBP_AUTOPATCHER_CHENXIAOLONG_HEADERS
    libdbp-autopatcher-chenxiaolong_global.h
    noobdevpatcher.h
    noobdevplugin.h
)

add_library(dbp-autopatcher-chenxiaolong SHARED ${DBP_AUTOPATCHER_CHENXIAOLONG_SOURCES})

set_target_properties(dbp-autopatcher-chenxiaolong PROPERTIES
    OUTPUT_NAME "dbp-autopatcher-chenxiaolong"
)

target_link_libraries(dbp-autopatcher-chenxiaolong
    Qt5::Core
    dbp
)

# Install library
install(
    TARGETS dbp-autopatcher-chenxiaolong
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)