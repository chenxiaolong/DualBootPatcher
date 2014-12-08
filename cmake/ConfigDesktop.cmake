include(GNUInstallDirs)

# Portable application

option(PORTABLE "Build as portable application" OFF)

if(WIN32)
    set(PORTABLE ON CACHE BOOL "Build as portable application" FORCE)
endif()

# Compiler flags

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # Enable C++11 support
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")

    # Enable warnings
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    # Except for "/*" within comment errors (present in doxygen blocks)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=comment")
endif()

# Use boost::regex instead on versions of gcc that don't have std::regex
# implemented

set(USE_BOOST_REGEX TRUE)

if(CMAKE_COMPILER_IS_GNUCXX)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                    OUTPUT_VARIABLE GCC_VERSION)
    if (GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
        message(STATUS "Found gcc >= 4.9; std::regex will be used")
        set(USE_BOOST_REGEX FALSE)
    endif()
endif()

set(BOOST_COMPONENTS filesystem system)

if(USE_BOOST_REGEX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_BOOST_REGEX")
    list(APPEND BOOST_COMPONENTS regex)
endif()

# Dependencies

find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

find_package(Qt5Core 5.3 REQUIRED)

find_package(LibArchive REQUIRED)
include_directories(${LibArchive_INCLUDE_DIRS})
link_directories(${LibArchive_LIBRARY_DIRS})

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})

# Install paths

if(${PORTABLE})
    set(BIN_INSTALL_DIR bin)
    set(DATA_INSTALL_DIR data)
    set(HEADERS_INSTALL_DIR include)

    if(WIN32)
        set(LIB_INSTALL_DIR bin)
    else()
        set(LIB_INSTALL_DIR lib)
    endif()
else()
    set(BIN_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
    set(DATA_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/mbp)
    set(HEADERS_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
endif()

# CPack

set(CPACK_PACKAGE_NAME "DualBootPatcher")
set(CPACK_PACKAGE_VERSION_MAJOR ${MBP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${MBP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${MBP_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A patcher for Android ROMs to make them multibootable")
set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2;ZIP")
