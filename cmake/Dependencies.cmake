# Dependencies

if(${MBP_BUILD_TARGET} STREQUAL system)
    include(cmake/dependencies/googletest.cmake)
    include(cmake/dependencies/libarchive.cmake)
    include(cmake/dependencies/liblzma.cmake)
    include(cmake/dependencies/lz4.cmake)
    include(cmake/dependencies/rapidjson.cmake)
    include(cmake/dependencies/zlib.cmake)
elseif(${MBP_BUILD_TARGET} STREQUAL device)
    # Always use static libraries
    backup_variable(CMAKE_FIND_LIBRARY_SUFFIXES)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

    include(cmake/dependencies/fuse.cmake)
    include(cmake/dependencies/googletest.cmake)
    include(cmake/dependencies/libarchive.cmake)
    include(cmake/dependencies/liblzma.cmake)
    include(cmake/dependencies/libsepol.cmake)
    include(cmake/dependencies/lz4.cmake)
    include(cmake/dependencies/qt5.cmake)
    include(cmake/dependencies/rapidjson.cmake)
    include(cmake/dependencies/zlib.cmake)

    restore_variable(CMAKE_FIND_LIBRARY_SUFFIXES)
endif()

# Needed for every target
include(cmake/dependencies/outcome.cmake)
include(cmake/dependencies/sodium.cmake)
