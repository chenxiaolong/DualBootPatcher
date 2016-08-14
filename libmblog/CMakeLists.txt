set(MBLOG_SOURCES
    src/logging.cpp
    src/stdio_logger.cpp
)

if(ANDROID)
    list(
        APPEND
        MBLOG_SOURCES
        src/android_logger.cpp
        src/kmsg_logger.cpp
    )
endif()

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    # Build static library

    add_library(mblog-static STATIC ${MBLOG_SOURCES})

    target_compile_definitions(
        mblog-static
        PRIVATE
        -DMB_LIBRARY
    )

    set_target_properties(
        mblog-static
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    if(NOT MSVC)
        set_target_properties(
            mblog-static
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()
elseif(${MBP_BUILD_TARGET} STREQUAL android-app
        OR ${MBP_BUILD_TARGET} STREQUAL desktop
        OR ${MBP_BUILD_TARGET} STREQUAL hosttools)
    # Build shared library

    add_library(mblog-shared SHARED ${MBLOG_SOURCES})

    target_compile_definitions(
        mblog-shared
        PRIVATE
        -DDMB_DYNAMIC_LINK
        -DMB_LIBRARY
    )

    set_target_properties(
        mblog-shared
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        OUTPUT_NAME mblog
    )

    if(NOT MSVC)
        set_target_properties(
            mblog-shared
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    if(ANDROID)
        target_link_libraries(
            mblog-shared
            log
        )
    endif()

    # Install shared library

    install(
        TARGETS mblog-shared
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )
endif()
