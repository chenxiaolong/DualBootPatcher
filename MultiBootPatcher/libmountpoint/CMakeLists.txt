include(ExternalProject)
include(${CMAKE_SOURCE_DIR}/cmake/LogicalCoreCount.cmake)

ExternalProject_Add(
    libmountpoint
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/libmountpoint
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/libmountpoint/out/
    BUILD_COMMAND ${NDK_BUILD}
                  -C <SOURCE_DIR>
                  -j${PROCESSOR_COUNT}
                  NDK_APP_LIBS_OUT=${CMAKE_BINARY_DIR}/libmountpoint/out
                  NDK_OUT=<TMP_DIR>
    INSTALL_COMMAND ""
)

# Always rebuild since files may have changed
ExternalProject_Add_Step(
    libmountpoint
    forcerebuild
    DEPENDEES update
    DEPENDERS configure
    ALWAYS 1
    COMMAND ""
)
