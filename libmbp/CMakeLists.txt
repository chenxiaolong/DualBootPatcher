set(MBP_SOURCES
    src/fileinfo.cpp
    src/patcherconfig.cpp
    # C wrapper API
    src/cwrapper/ccommon.cpp
    src/cwrapper/cfileinfo.cpp
    src/cwrapper/cpatcherconfig.cpp
    src/cwrapper/cpatcherinterface.cpp
    src/cwrapper/private/util.cpp
    # Edify tokenizer
    src/edify/tokenizer.cpp
    # Private classes
    src/private/fileutils.cpp
    src/private/miniziputils.cpp
    src/private/stringutils.cpp
    # Autopatchers
    src/autopatchers/standardpatcher.cpp
    src/autopatchers/mountcmdpatcher.cpp
    # Patchers
    src/patchers/multibootpatcher.cpp
    src/patchers/odinpatcher.cpp
    src/patchers/ramdiskupdater.cpp
    # SHA1 library
    external/sha.cpp
)

if(WIN32)
    list(APPEND MBP_SOURCES
        src/private/win32.cpp
    )
endif()

set(variants)

if(MBP_TARGET_HAS_BUILDS)
    list(APPEND variants static)
endif()
if(${MBP_BUILD_TARGET} STREQUAL android-app
        OR ${MBP_BUILD_TARGET} STREQUAL desktop)
    list(APPEND variants shared)
endif()

# Build libraries
foreach(variant ${variants})
    set(lib_target mbp-${variant})

    # Build library
    if(${variant} STREQUAL shared)
        add_library(${lib_target} SHARED ${MBP_SOURCES})
    elseif(${variant} STREQUAL static)
        add_library(${lib_target} STATIC ${MBP_SOURCES})
    endif()

    # Includes
    target_include_directories(
        ${lib_target}
        PUBLIC
        include
        PRIVATE
        .
        ${MBP_LIBARCHIVE_INCLUDES}
        ${MBP_LIBLZMA_INCLUDES}
        ${MBP_ZLIB_INCLUDES}
        ${CMAKE_SOURCE_DIR}/external
        ${CMAKE_CURRENT_BINARY_DIR}/include
    )

    # Only build static library if needed
    if(${variant} STREQUAL static)
        set_target_properties(${lib_target} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()

    # minizip type safety
    target_compile_definitions(${lib_target} PRIVATE -DSTRICTZIPUNZIP)

    # Export symbols
    target_compile_definitions(${lib_target} PRIVATE -DMB_LIBRARY)

    # Win32 DLL export
    if(${variant} STREQUAL shared)
        target_compile_definitions(${lib_target} PRIVATE -DMB_DYNAMIC_LINK)
    endif()

    # Build with -fPIC
    set_target_properties(${lib_target} PROPERTIES POSITION_INDEPENDENT_CODE 1)

    # Target C++11
    if(NOT MSVC)
        set_target_properties(
            ${lib_target}
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbp)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PUBLIC
        mbcommon-${variant}
        mbdevice-${variant}
        PRIVATE
        mbpio-static
        mblog-${variant}
        minizip-${variant}
        ${MBP_LIBARCHIVE_LIBRARIES}
        ${MBP_LIBLZMA_LIBRARIES}
        ${MBP_ZLIB_LIBRARIES}
    )

    if(${MBP_BUILD_TARGET} STREQUAL android-app)
        target_link_libraries(${lib_target} PRIVATE c_compat)
    endif()

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
