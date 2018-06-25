set(variants)

if(MBP_TARGET_HAS_BUILDS
        AND NOT ${MBP_BUILD_TARGET} STREQUAL hosttools)
    list(APPEND variants static)
endif()
if(${MBP_BUILD_TARGET} STREQUAL android-app
        OR ${MBP_BUILD_TARGET} STREQUAL desktop)
    list(APPEND variants shared)
endif()

# Build libraries
foreach(variant ${variants})
    set(lib_target mbpatcher-${variant})
    string(TOUPPER ${variant} uvariant)

    # Build library
    add_library(
        ${lib_target}
        ${uvariant}
        src/fileinfo.cpp
        src/patcherconfig.cpp
        # C wrapper API
        src/cwrapper/cfileinfo.cpp
        src/cwrapper/cpatcherconfig.cpp
        src/cwrapper/cpatcherinterface.cpp
        # Edify tokenizer
        src/edify/tokenizer.cpp
        # Private classes
        src/private/fileutils.cpp
        src/private/miniziputils.cpp
        # Autopatchers
        src/autopatchers/magiskpatcher.cpp
        src/autopatchers/mountcmdpatcher.cpp
        src/autopatchers/standardpatcher.cpp
        # Patchers
        src/patchers/odinpatcher.cpp
        src/patchers/ramdiskupdater.cpp
        src/patchers/zippatcher.cpp
    )

    # Includes
    target_include_directories(
        ${lib_target}
        PUBLIC
        include
        PRIVATE
        .
        ${CMAKE_SOURCE_DIR}/external/minizip
    )

    # Only build static library if needed
    if(${variant} STREQUAL static)
        set_target_properties(${lib_target} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbpatcher)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PUBLIC
        mbcommon-${variant}
        mbdevice-${variant}
        PRIVATE
        interface.global.CXXVersion
        interface.mbcommon.library
        $<$<STREQUAL:${variant},shared>:interface.mbcommon.dynamic-link>
        mbpio-${variant}
        mblog-${variant}
        libminizip
        LibArchive::LibArchive
    )

    if(UNIX AND NOT ANDROID)
        target_link_libraries(${lib_target} PRIVATE pthread)
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
