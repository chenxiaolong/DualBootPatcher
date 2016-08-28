if(NOT ${MBP_BUILD_TARGET} STREQUAL android-system)
    return()
endif()

include_directories(${MBP_LIBARCHIVE_INCLUDES})

# cryptfstool

add_executable(
    cryptfstool
    main.cpp
)

set_target_properties(
    cryptfstool
    PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED 1
    POSITION_INDEPENDENT_CODE 1
    LINK_FLAGS "-static"
    LINK_SEARCH_START_STATIC ON
)

target_link_libraries(
    cryptfstool
    mbp-static
    mbpio-static
    mbutil-static
    mblog-static
    mbcommon-static
    ${MBP_LIBARCHIVE_LIBRARIES}
    ${MBP_ZLIB_LIBRARIES}
    ${MBP_LZ4_LIBRARIES}
    ${MBP_LZO_LIBRARIES}
    ${MBP_LIBLZMA_LIBRARIES}
)

target_compile_options(
    cryptfstool
    PRIVATE
    -fPIE
)

install(
    TARGETS cryptfstool
    RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
    COMPONENT Applications
)

# cryptfstool_rec

add_executable(
    cryptfstool_rec
    main_rec.cpp
)

set_target_properties(
    cryptfstool_rec
    PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED 1
    POSITION_INDEPENDENT_CODE 1
    LINK_FLAGS "-pie -Wl,--dynamic-linker=/sbin/linker"
)

target_link_libraries(
    cryptfstool_rec
    mbutil-static
    mblog-static
    mbcommon-static
)

install(
    TARGETS cryptfstool_rec
    RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
    COMPONENT Applications
)
