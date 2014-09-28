add_definitions(
    -DLIBDBPAUTOPATCHERPATCHFILE_LIBRARY
)

add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_AUTOPATCHER_PATCHFILE_SOURCES
    patchfilepatcher.cpp
    patchfileplugin.cpp
)

set(DBP_AUTOPATCHER_PATCHFILE_PRIVATE_HEADERS
    patchfilepatcher_p.h
)

set(DBP_AUTOPATCHER_PATCHFILE_HEADERS
    libdbp-autopatcher-patchfile_global.h
    patchfilepatcher.h
    patchfileplugin.h
)

add_library(dbp-autopatcher-patchfile SHARED ${DBP_AUTOPATCHER_PATCHFILE_SOURCES})

set_target_properties(dbp-autopatcher-patchfile PROPERTIES
    OUTPUT_NAME "dbp-autopatcher-patchfile"
)

target_link_libraries(dbp-autopatcher-patchfile
    Qt5::Core
    dbp
)

# Install library
install(
    TARGETS dbp-autopatcher-patchfile
    DESTINATION ${PLUGINS_INSTALL_DIR}
    COMPONENT Plugins
)