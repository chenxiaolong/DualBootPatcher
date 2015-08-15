include(ExternalProject)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/version.h
    @ONLY
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.mk
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
                  APP_BUILD_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/Android.mk
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

add_dependencies(
    mbtool
    mbp-mini
    mbpio-android
)

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out/
    DESTINATION ${DATA_INSTALL_DIR}/binaries/android
    COMPONENT Libraries
)
