set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Allow libdbp headers to be found
include_directories(${CMAKE_SOURCE_DIR})

add_definitions(
    -DLIBDBP_LIBRARY
    -DLIBDBP_VERSION="${DBP_VERSION}"
)

set(DBP_SOURCES
    bootimage.cpp
    cpiofile.cpp
    device.cpp
    fileinfo.cpp
    partitionconfig.cpp
    patchererror.cpp
    patcherpaths.cpp
    patchinfo.cpp
    private/fileutils.cpp
    cwrapper/cbootimage.cpp
    cwrapper/ccpiofile.cpp
    cwrapper/cdevice.cpp
    cwrapper/cfileinfo.cpp
    cwrapper/cpartitionconfig.h
    cwrapper/cpatchinfo.cpp
)

set(DBP_HEADERS
    bootimage.h
    cpiofile.h
    device.h
    fileinfo.h
    libdbp_global.h
    partitionconfig.h
    patchererror.h
    patcherpaths.h
    patcherinterface.h
    patchinfo.h
)

################################################################################
# jflte autopatcher
################################################################################

set(DBP_AUTOPATCHER_JFLTE_SOURCES
    autopatchers/jflte/jfltepatcher.cpp
)

set(DBP_AUTOPATCHER_JFLTE_HEADERS
    autopatchers/jflte/jfltepatcher.h
)

################################################################################
# Noobdev autopatcher
################################################################################

set(DBP_AUTOPATCHER_NOOBDEV_SOURCES
    autopatchers/noobdev/noobdevpatcher.cpp
)

set(DBP_AUTOPATCHER_NOOBDEV_HEADERS
    autopatchers/noobdev/noobdevpatcher.h
)

################################################################################
# Patch file autopatcher
################################################################################

set(DBP_AUTOPATCHER_PATCHFILE_SOURCES
    autopatchers/patchfile/patchfilepatcher.cpp
)

set(DBP_AUTOPATCHER_PATCHFILE_HEADERS
    autopatchers/patchfile/patchfilepatcher.h
)

################################################################################
# Standard autopatcher
################################################################################

set(DBP_AUTOPATCHER_STANDARD_SOURCES
    autopatchers/standard/standardpatcher.cpp
)

set(DBP_AUTOPATCHER_STANDARD_HEADERS
    autopatchers/standard/standardpatcher.h
)

################################################################################
# Multiboot patcher
################################################################################

set(DBP_PATCHER_MULTIBOOT_SOURCES
    patchers/multiboot/multibootpatcher.cpp
)

set(DBP_PATCHER_MULTIBOOT_HEADERS
    patchers/multiboot/multibootpatcher.h
)

################################################################################
# Primary upgrade patcher
################################################################################

set(DBP_PATCHER_PRIMARYUPGRADE_SOURCES
    patchers/primaryupgrade/primaryupgradepatcher.cpp
)

set(DBP_PATCHER_PRIMARYUPGRADE_HEADERS
    patchers/primaryupgrade/primaryupgradepatcher.h
)

################################################################################
# Syncdaemon update patcher
################################################################################

set(DBP_PATCHER_SYNCDAEMONUPDATE_SOURCES
    patchers/syncdaemonupdate/syncdaemonupdatepatcher.cpp
)

set(DBP_PATCHER_SYNCDAEMONUPDATE_HEADERS
    patchers/syncdaemonupdate/syncdaemonupdatepatcher.h
)

################################################################################
# bacon (OnePlus One) ramdisk patcher
################################################################################

set(DBP_RAMDISK_BACON_SOURCES
    ramdiskpatchers/bacon/baconramdiskpatcher.cpp
)

set(DBP_RAMDISK_BACON_HEADERS
    ramdiskpatchers/bacon/baconramdiskpatcher.h
)

################################################################################
# Common ramdisk patcher classes
################################################################################

set(DBP_RAMDISK_COMMON_SOURCES
    ramdiskpatchers/common/coreramdiskpatcher.cpp
)

set(DBP_RAMDISK_COMMON_HEADERS
    ramdiskpatchers/common/coreramdiskpatcher.h
)

################################################################################
# d800 (LG G2) ramdisk patcher
################################################################################

set(DBP_RAMDISK_D800_SOURCES
    ramdiskpatchers/d800/d800ramdiskpatcher.cpp
)

set(DBP_RAMDISK_D800_HEADERS
    ramdiskpatchers/d800/d800ramdiskpatcher.h
)

################################################################################
# falcon (Motorola Moto G) ramdisk patcher
################################################################################

set(DBP_RAMDISK_FALCON_SOURCES
    ramdiskpatchers/falcon/falconramdiskpatcher.cpp
)

set(DBP_RAMDISK_FALCON_HEADERS
    ramdiskpatchers/falcon/falconramdiskpatcher.h
)

################################################################################
# Common Samsung Galaxy ramdisk patcher classes
################################################################################

set(DBP_RAMDISK_GALAXY_SOURCES
    ramdiskpatchers/galaxy/galaxyramdiskpatcher.cpp
)

set(DBP_RAMDISK_GALAXY_HEADERS
    ramdiskpatchers/galaxy/galaxyramdiskpatcher.h
)

################################################################################
# hammerhead (Google/LG Nexus 5) ramdisk patcher
################################################################################

set(DBP_RAMDISK_HAMMERHEAD_SOURCES
    ramdiskpatchers/hammerhead/hammerheadramdiskpatcher.cpp
)

set(DBP_RAMDISK_HAMMERHEAD_HEADERS
    ramdiskpatchers/hammerhead/hammerheadramdiskpatcher.h
)

################################################################################
# hlte (Samsung Galaxy Note 3) ramdisk patcher
################################################################################

set(DBP_RAMDISK_HLTE_SOURCES
    ramdiskpatchers/hlte/hlteramdiskpatcher.cpp
)

set(DBP_RAMDISK_HLTE_HEADERS
    ramdiskpatchers/hlte/hlteramdiskpatcher.h
)

################################################################################
# jflte (Samsung Galaxy S 4) ramdisk patcher
################################################################################

set(DBP_RAMDISK_JFLTE_SOURCES
    ramdiskpatchers/jflte/jflteramdiskpatcher.cpp
)

set(DBP_RAMDISK_JFLTE_HEADERS
    ramdiskpatchers/jflte/jflteramdiskpatcher.h
)

################################################################################
# klte (Samsung Galaxy S 5) ramdisk patcher
################################################################################

set(DBP_RAMDISK_KLTE_SOURCES
    ramdiskpatchers/klte/klteramdiskpatcher.cpp
)

set(DBP_RAMDISK_KLTE_HEADERS
    ramdiskpatchers/klte/klteramdiskpatcher.h
)

################################################################################
# Common qcom ramdisk patcher classes
################################################################################

set(DBP_RAMDISK_QCOM_SOURCES
    ramdiskpatchers/qcom/qcomramdiskpatcher.cpp
)

set(DBP_RAMDISK_QCOM_HEADERS
    ramdiskpatchers/qcom/qcomramdiskpatcher.h
)

################################################################################
# libdbp
################################################################################

add_library(dbp SHARED
            ${DBP_SOURCES}
            ${DBP_AUTOPATCHER_JFLTE_SOURCES}
            ${DBP_AUTOPATCHER_NOOBDEV_SOURCES}
            ${DBP_AUTOPATCHER_PATCHFILE_SOURCES}
            ${DBP_AUTOPATCHER_STANDARD_SOURCES}
            ${DBP_PATCHER_MULTIBOOT_SOURCES}
            ${DBP_PATCHER_PRIMARYUPGRADE_SOURCES}
            ${DBP_PATCHER_SYNCDAEMONUPDATE_SOURCES}
            ${DBP_RAMDISK_BACON_SOURCES}
            ${DBP_RAMDISK_COMMON_SOURCES}
            ${DBP_RAMDISK_D800_SOURCES}
            ${DBP_RAMDISK_FALCON_SOURCES}
            ${DBP_RAMDISK_GALAXY_SOURCES}
            ${DBP_RAMDISK_HAMMERHEAD_SOURCES}
            ${DBP_RAMDISK_HLTE_SOURCES}
            ${DBP_RAMDISK_JFLTE_SOURCES}
            ${DBP_RAMDISK_KLTE_SOURCES}
            ${DBP_RAMDISK_QCOM_SOURCES})

if(NOT ANDROID)
    set_target_properties(dbp PROPERTIES
        VERSION 1.0.0
        SOVERSION 1
    )
endif()

target_link_libraries(dbp
    ${Boost_LIBRARIES}
    ${LibArchive_LIBRARIES}
    ${LIBXML2_LIBRARIES}
)

# Install library
install(
    TARGETS dbp
    LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
)

# Install headers
install(
    FILES ${DBP_HEADERS}
    DESTINATION ${HEADERS_INSTALL_DIR}/libdbp
    COMPONENT Development
)
