if(${MBP_BUILD_TARGET} STREQUAL android-app)
    set(MISCSTUFF_JNI_SOURCES
        constants.cpp
        jni_load.cpp
        libmiscstuff.cpp
    )

    add_library(miscstuff-jni SHARED ${MISCSTUFF_JNI_SOURCES})

    target_compile_options(
        miscstuff-jni
        PRIVATE
        -Wno-missing-prototypes
    )

    target_link_libraries(
        miscstuff-jni
        interface.global.CXXVersion
        interface.mbcommon.library
        interface.mbcommon.dynamic-link
        mbbootimg-shared
        mblog-shared
        LibArchive::LibArchive
    )

    install(
        TARGETS miscstuff-jni
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )
endif()
