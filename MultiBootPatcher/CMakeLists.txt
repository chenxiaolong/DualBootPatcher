cmake_minimum_required(VERSION 3.0)

project(DualBootPatcher)
set(DBP_VERSION_MAJOR 9)
set(DBP_VERSION_MINOR 0)
set(DBP_VERSION_PATCH 0Alpha6)
set(DBP_VERSION "${DBP_VERSION_MAJOR}.${DBP_VERSION_MINOR}.${DBP_VERSION_PATCH}")

option(BUILD_ANDROID "Build for Android" OFF)

if(BUILD_ANDROID)
    include(cmake/ConfigAndroid.cmake)
else()
    include(cmake/ConfigDesktop.cmake)
endif()

# Main patcher
add_subdirectory(libdbp)
add_subdirectory(data)

if(BUILD_ANDROID)
    add_subdirectory(syncdaemon)
    add_subdirectory(libmountpoint)
    add_subdirectory(libloki)
else()
    add_subdirectory(shortcuts)
    add_subdirectory(gui)
endif()

# Third party binaries
add_subdirectory(thirdparty)

# CPack stuff

# CPack removes the prefix when archive component install is used, unfortunately
#set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
#set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} Libraries Applications)

set(CPACK_PACKAGE_NAME "DualBootPatcher")
set(CPACK_PACKAGE_VERSION_MAJOR ${DBP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${DBP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${DBP_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A patcher for Android ROMs to make them dualbootable")

set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2;ZIP")

include(CPack)
