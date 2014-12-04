include(ExternalProject)
include(${CMAKE_SOURCE_DIR}/cmake/LogicalCoreCount.cmake)


configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk.in
    #${CMAKE_CURRENT_BINARY_DIR}/Android.mk
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk
    @ONLY
)

ExternalProject_Add(
    mbtool
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out/
    BUILD_COMMAND ${NDK_BUILD}
                  -C <SOURCE_DIR>
                  -j${PROCESSOR_COUNT}
                  NDK_PROJECT_PATH=.
                  NDK_APPLICATION_MK=Application.mk
                  NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out
                  NDK_OUT=<TMP_DIR>
                  ${NDK_ARGS}
    INSTALL_COMMAND ""
)

# Always rebuild since files may have changed
ExternalProject_Add_Step(
    mbtool
    forcerebuild
    DEPENDEES update
    DEPENDERS configure
    ALWAYS 1
    COMMAND ""
)

install(
    DIRECTORY ${CMAKE_BINARY_DIR}/mbtool/out/
    DESTINATION ${DATA_INSTALL_DIR}/binaries/android
    COMPONENT Libraries
)
