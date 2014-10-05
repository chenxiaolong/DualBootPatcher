set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_definitions(
    -DLIBDBP_LIBRARY
)

add_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

set(DBP_SOURCES
    bootimage.cpp
    partitionconfig.cpp
    patchinfo.cpp
    cpiofile.cpp
    fileinfo.cpp
    patcherpaths.cpp
    device.cpp
    patchererror.cpp
    plugininterface.cpp
)

set(DBP_PRIVATE_HEADERS
    bootimage_p.h
    cpiofile_p.h
    device_p.h
    fileinfo_p.h
    partitionconfig_p.h
    patcherpaths_p.h
    patchinfo_p.h
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
    patchinfo.h
    plugininterface.h
)

add_library(dbp SHARED ${DBP_SOURCES})

if(NOT ANDROID)
    set_target_properties(dbp PROPERTIES
        VERSION 1.0.0
        SOVERSION 1
    )
endif()

target_link_libraries(dbp
    Qt5::Core
    ${LibArchive_LIBRARIES}
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
