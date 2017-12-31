find_package(LibArchive REQUIRED)

add_library(LibArchive::LibArchive UNKNOWN IMPORTED)
set_target_properties(
    LibArchive::LibArchive
    PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    IMPORTED_LOCATION "${LibArchive_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${LibArchive_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "LibLZMA::LibLZMA;LZ4::LZ4;ZLIB::ZLIB"
)
