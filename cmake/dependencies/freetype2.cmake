find_package(Freetype REQUIRED)

if(CMAKE_VERSION VERSION_LESS 3.10)
    add_library(Freetype::Freetype UNKNOWN IMPORTED)
    set_target_properties(
        Freetype::Freetype
        PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${FREETYPE_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIRS}"
    )
endif()
