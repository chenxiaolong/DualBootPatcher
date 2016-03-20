include_directories(${MBP_LIBARCHIVE_LOWAPI_INCLUDES})

if(${MBP_BUILD_TARGET} STREQUAL android-app)
    set(MISCSTUFF_SOURCES
        libmiscstuff.cpp
        mntent.c
    )

    add_definitions(-fvisibility=default)

    add_library(miscstuff SHARED ${MISCSTUFF_SOURCES})

    set_target_properties(
        miscstuff
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    if(NOT MSVC)
        set_target_properties(
            miscstuff
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    target_link_libraries(
        miscstuff
        mblog-shared
        ${MBP_LIBARCHIVE_LOWAPI_LIBRARIES}
        ${MBP_LIBLZMA_LIBRARIES}
        ${MBP_ZLIB_LIBRARIES}
    )

    install(
        TARGETS miscstuff
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )
endif()
