set(variants)

if(MBP_TARGET_HAS_BUILDS)
    list(APPEND variants static)
endif()
if(${MBP_BUILD_TARGET} STREQUAL android-app
        OR ${MBP_BUILD_TARGET} STREQUAL desktop
        OR ${MBP_BUILD_TARGET} STREQUAL hosttools)
    list(APPEND variants shared)
endif()

# Build libraries
foreach(variant ${variants})
    set(lib_target mblog-${variant})
    string(TOUPPER ${variant} uvariant)

    # Build library
    add_library(
        ${lib_target}
        ${uvariant}
        src/base_logger.cpp
        src/logging.cpp
        src/stdio_logger.cpp
    )

    if(ANDROID)
        target_sources(
            ${lib_target}
            PRIVATE
            src/android_logger.cpp
            src/kmsg_logger.cpp
        )
    endif()

    # Includes
    target_include_directories(
        ${lib_target}
        PUBLIC include
    )

    # Only build static library if needed
    if(${variant} STREQUAL static)
        set_target_properties(${lib_target} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()

    # Ignore non-literal format strings
    if(NOT MSVC)
        target_compile_options(${lib_target} PRIVATE -Wno-format-nonliteral)
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mblog)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PUBLIC
        mbcommon-${variant}
        PRIVATE
        interface.global.CXXVersion
        interface.mbcommon.library
        $<$<STREQUAL:${variant},shared>:interface.mbcommon.dynamic-link>
    )

    if(ANDROID AND ${variant} STREQUAL shared)
        target_link_libraries(
            ${lib_target}
            PUBLIC log
        )
    endif()

    # Install shared library
    if(${variant} STREQUAL shared)
        install(
            TARGETS ${lib_target}
            LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
            RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
            #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        )
    endif()
endforeach()
