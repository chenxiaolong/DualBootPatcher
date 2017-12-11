include(GNUInstallDirs)

# Portable application
option(MBP_PORTABLE "Build as portable application" OFF)
if(WIN32)
    set(MBP_PORTABLE ON CACHE BOOL "Build as portable application" FORCE)
endif()

# Install paths

if(${MBP_PORTABLE})
    set(DATA_INSTALL_DIR data)
    set(HEADERS_INSTALL_DIR include)

    set(BIN_INSTALL_DIR .)
    if(WIN32)
        set(LIB_INSTALL_DIR .)
    else()
        set(LIB_INSTALL_DIR lib)
    endif()
else()
    set(BIN_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
    set(DATA_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/mbp)
    set(HEADERS_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
endif()

# CPack

set(CPACK_PACKAGE_NAME "DualBootPatcher")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A patcher for Android ROMs to make them multibootable")
set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2;ZIP")
