set(include_dirs
    .
    ..
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${MBP_FREETYPE2_INCLUDES}
    ${MBP_LIBPNG_INCLUDES}
    ${MBP_ZLIB_INCLUDES}
    ${MBP_LIBCUTILS_INCLUDES}
    ${MBP_LIBLOG_INCLUDES}
    ${MBP_LIBPIXELFLINGER_INCLUDES}
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
        POSITION_INDEPENDENT_CODE 1
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
        POSITION_INDEPENDENT_CODE 1
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
        POSITION_INDEPENDENT_CODE 1
    )

    target_include_directories(
        minui-backend-drm
        BEFORE
        PRIVATE
        ${MBP_LIBDRM_INCLUDES}
        ${MBP_LIBDRM_INCLUDES}/drm
    )

    list(APPEND MINUI_BACKEND_OBJECTS $<TARGET_OBJECTS:minui-backend-drm>)
    list(APPEND MINUI_BACKEND_LIBRARIES ${MBP_LIBDRM_LIBRARIES})
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
        POSITION_INDEPENDENT_CODE 1
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
    POSITION_INDEPENDENT_CODE 1
)

target_link_libraries(
    mbbootui-minui
    PRIVATE
    ${MINUI_BACKEND_LIBRARIES}
    ${MBP_FREETYPE2_LIBRARIES}
    ${MBP_LIBPNG_LIBRARIES}
    ${MBP_ZLIB_LIBRARIES}
    ${MBP_LIBLOG_LIBRARIES}
    ${MBP_LIBPIXELFLINGER_LIBRARIES}
    ${MBP_LIBCUTILS_LIBRARIES}
)
