if(${MBP_BUILD_TARGET} STREQUAL android-app)
    set(MISCSTUFF_JNI_SOURCES
        constants.cpp
        jni_load.cpp
        libmiscstuff.cpp
    )

    add_library(miscstuff-jni SHARED ${MISCSTUFF_JNI_SOURCES})

    target_include_directories(
        miscstuff-jni
        PRIVATE
        ${MBP_LIBARCHIVE_INCLUDES}
    )

    set_target_properties(
        miscstuff-jni
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
    )

    target_compile_definitions(
        miscstuff-jni
        PRIVATE
        -DMB_DYNAMIC_LINK
        -DMB_LIBRARY
    )

    if(NOT MSVC)
        set_target_properties(
            miscstuff-jni
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    target_link_libraries(
        miscstuff-jni
        mbbootimg-shared
        mblog-shared
        ${MBP_LIBARCHIVE_LIBRARIES}
        ${MBP_LIBLZMA_LIBRARIES}
        ${MBP_LZ4_LIBRARIES}
        ${MBP_ZLIB_LIBRARIES}
    )

    install(
        TARGETS miscstuff-jni
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )
endif()
