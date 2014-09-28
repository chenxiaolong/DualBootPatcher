add_definitions(
    -DLIBDBPAUTOPATCHERJFLTE_LIBRARY
)

add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_AUTOPATCHER_JFLTE_SOURCES
    jfltepatcher.cpp
    jflteplugin.cpp
)

set(DBP_AUTOPATCHER_JFLTE_PRIVATE_HEADERS
    jfltepatcher_p.h
)

set(DBP_AUTOPATCHER_JFLTE_HEADERS
    libdbp-autopatcher-jflte_global.h
    jfltepatcher.h
    jflteplugin.h
)

add_library(dbp-autopatcher-jflte SHARED ${DBP_AUTOPATCHER_JFLTE_SOURCES})

set_target_properties(dbp-autopatcher-jflte PROPERTIES
    OUTPUT_NAME "dbp-autopatcher-jflte"
)

target_link_libraries(dbp-autopatcher-jflte
    Qt5::Core
    dbp
    dbp-autopatcher-standard
)

# Set rpath for portable build
if (${PORTABLE})
    set_target_properties(dbp-autopatcher-jflte
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        INSTALL_RPATH "\$ORIGIN"
    )
endif()

# Install library
install(
    TARGETS dbp-autopatcher-jflte
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)