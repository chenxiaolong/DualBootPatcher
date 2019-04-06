set(variants)

list(APPEND variants static)
if(${MBP_BUILD_TARGET} STREQUAL system)
    list(APPEND variants shared)
endif()

# Build libraries
foreach(variant ${variants})
    set(lib_target mbpio-${variant})
    string(TOUPPER ${variant} uvariant)

    # Build library
    add_library(
        ${lib_target}
        ${uvariant}
        src/directory.cpp
        src/path.cpp
    )

    # Includes
    target_include_directories(${lib_target} PUBLIC include)

    # Only build static library if needed
    if(${variant} STREQUAL static)
        set_target_properties(${lib_target} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()

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
