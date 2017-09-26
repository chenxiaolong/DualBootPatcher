# target_link_libraries() can't be used with object libraries
get_target_property(mbcommon_include mbcommon-static INCLUDE_DIRECTORIES)
get_target_property(mbdevice_include mbdevice-static INCLUDE_DIRECTORIES)

set(include_dirs
    .
    ..
    ${mbcommon_include}
    ${mbdevice_include}
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

# Uncomment to enable event logging
#add_definitions(-D_EVENT_LOGGING)

# Uncomment to enable JPEG support (currently not working)
#include_directories(${MBP_LIBJPEG_INCLUDES})
#add_definitions(-DTW_INCLUDE_JPEG)
#target_link_libraries(...)

add_compile_options(
    -Wno-c99-extensions
    -Wno-sign-compare
)

# Backends
set(ENABLE_OVERLAY_MSM_OLD_BACKEND TRUE)
set(ENABLE_ADF_BACKEND FALSE)
set(ENABLE_DRM_BACKEND TRUE)
set(ENABLE_FBDEV_BACKEND TRUE)

make_directory(${CMAKE_CURRENT_BINARY_DIR}/include/backend)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/backend.gen.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/backend/backend.gen.h
)

set(MINUI_BACKEND_OBJECTS)
set(MINUI_BACKEND_LIBRARIES)

# Overlay backend (MSM old)
if(ENABLE_OVERLAY_MSM_OLD_BACKEND)
    add_library(
        minui-backend-overlay-msm-old
        OBJECT
        backend/backend_overlay.cpp
    )

    target_include_directories(
        minui-backend-overlay-msm-old
        PRIVATE
        ${include_dirs}
    )

    set_target_properties(
        minui-backend-overlay-msm-old
        PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED 1
        LINK_LIBRARIES AndroidSystemCore::PixelFlinger
    )

    target_compile_definitions(
        minui-backend-overlay-msm-old
        PRIVATE
        -DMSM_BSP
        -DBACKEND_NAME=overlay_msm_old
    )

    target_include_directories(
        minui-backend-overlay-msm-old
        BEFORE
        PRIVATE
        include/msm_old
    )

    list(APPEND MINUI_BACKEND_OBJECTS
         $<TARGET_OBJECTS:minui-backend-overlay-msm-old>)
endif()

# ADF backend
if(ENABLE_ADF_BACKEND)
    add_library(
        minui-backend-adf
        OBJECT
        backend/backend_adf.cpp
    )

    target_include_directories(
        minui-backend-adf
        PRIVATE
        ${include_dirs}
    )

    set_target_properties(
        minui-backend-adf
        PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED 1
        LINK_LIBRARIES AndroidSystemCore::PixelFlinger
    )

    list(APPEND MINUI_BACKEND_OBJECTS $<TARGET_OBJECTS:minui-backend-adf>)
    list(APPEND MINUI_BACKEND_LIBRARIES ${MBP_LIBADF_LIBRARIES})
endif()

# DRM backend
if(ENABLE_DRM_BACKEND)
    add_library(
        minui-backend-drm
        OBJECT
        backend/backend_drm.cpp
    )

    target_include_directories(
        minui-backend-drm
        PRIVATE
        ${include_dirs}
    )

    set_target_properties(
        minui-backend-drm
        PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED 1
        LINK_LIBRARIES "AndroidSystemCore::PixelFlinger;LibDrm::LibDrm"
    )

    list(APPEND MINUI_BACKEND_OBJECTS $<TARGET_OBJECTS:minui-backend-drm>)
endif()

# FBDEV backend
if(ENABLE_FBDEV_BACKEND)
    add_library(
        minui-backend-fbdev
        OBJECT
        backend/backend_fbdev.cpp
    )

    target_include_directories(
        minui-backend-fbdev
        PRIVATE
        ${include_dirs}
    )

    set_target_properties(
        minui-backend-fbdev
        PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED 1
        LINK_LIBRARIES AndroidSystemCore::PixelFlinger
    )

    list(APPEND MINUI_BACKEND_OBJECTS $<TARGET_OBJECTS:minui-backend-fbdev>)
endif()

# Main library
add_library(
    mbbootui-minui
    STATIC
    events.cpp
    graphics.cpp
    graphics_utils.cpp
    truetype.cpp
    resources.cpp
    backend/backend.cpp
    ${MINUI_BACKEND_OBJECTS}
)

target_include_directories(
    mbbootui-minui
    PRIVATE
    ${include_dirs}
)

set_target_properties(
    mbbootui-minui
    PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED 1
)

target_link_libraries(
    mbbootui-minui
    PRIVATE
    ${MINUI_BACKEND_LIBRARIES}
    AndroidSystemCore::Cutils
    AndroidSystemCore::PixelFlinger
    Freetype::Freetype
    LibDrm::LibDrm # TODO: If DRM is enabled
    PNG::PNG
)
