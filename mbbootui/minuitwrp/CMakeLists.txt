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

add_library(
    mbbootui-minui
    STATIC
    events.cpp
    graphics.cpp
    graphics_utils.cpp
    truetype.cpp
    resources.cpp
    backend/backend.cpp
)

target_include_directories(
    mbbootui-minui
    PRIVATE
    .
    ..
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

# Uncomment to enable event logging
#target_compile_definitions(mbbootui-minui PRIVATE -D_EVENT_LOGGING)

# Uncomment to enable JPEG support (currently not working)
#target_include_directories(mbbootui-minui PRIVATE ${MBP_LIBJPEG_INCLUDES})
#target_compile_definitions(mbbootui-minui PRIVATE -DTW_INCLUDE_JPEG)
#target_link_libraries(mbbootui-minui PRIVATE ${MBP_LIBJPEG_LIBRARIES})

target_compile_options(
    mbbootui-minui
    PRIVATE
    -Wno-c99-extensions
    -Wno-sign-compare
)

target_link_libraries(
    mbbootui-minui
    PRIVATE
    interface.global.CXXVersion
    mbbootui-config
    mbutil-static
    AndroidSystemCore::Cutils
    Freetype::Freetype
    PNG::PNG
)

# Overlay backend (MSM old)
if(ENABLE_OVERLAY_MSM_OLD_BACKEND)
    target_sources(
        mbbootui-minui
        PRIVATE
        backend/backend_overlay.cpp
    )

    target_link_libraries(
        mbbootui-minui
        PRIVATE
        AndroidSystemCore::PixelFlinger
    )

    set_source_files_properties(
        backend/backend_overlay.cpp
        PROPERTIES
        COMPILE_FLAGS "-DMSM_BSP -DBACKEND_NAME=overlay_msm_old"
    )

    target_include_directories(
        mbbootui-minui
        BEFORE
        PRIVATE
        include/msm_old
    )
endif()

# ADF backend
if(ENABLE_ADF_BACKEND)
    target_sources(
        mbbootui-minui
        PRIVATE
        backend/backend_adf.cpp
    )

    target_link_libraries(
        mbbootui-minui
        PRIVATE
        AndroidSystemCore::PixelFlinger
        LibAdf::LibAdf
    )
endif()

# DRM backend
if(ENABLE_DRM_BACKEND)
    target_sources(
        mbbootui-minui
        PRIVATE
        backend/backend_drm.cpp
    )

    target_link_libraries(
        mbbootui-minui
        PRIVATE
        AndroidSystemCore::PixelFlinger
        LibDrm::LibDrm
    )
endif()

# FBDEV backend
if(ENABLE_FBDEV_BACKEND)
    target_sources(
        mbbootui-minui
        PRIVATE
        backend/backend_fbdev.cpp
    )

    target_link_libraries(
        mbbootui-minui
        PRIVATE
        AndroidSystemCore::PixelFlinger
    )
endif()
