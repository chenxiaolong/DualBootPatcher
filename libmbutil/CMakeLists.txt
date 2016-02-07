include(ExternalProject)

include_directories(include)

# Generate Android.mk
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.mk
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.include.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.include.mk
    @ONLY
)

ExternalProject_Add(
    mbutil
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out/
    BUILD_COMMAND ${NDK_BUILD}
                  -C <SOURCE_DIR>
                  NDK_PROJECT_PATH=.
                  NDK_APPLICATION_MK=Application.mk
                  NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out
                  NDK_OUT=<TMP_DIR>
                  APP_BUILD_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/Android.mk
                  ${NDK_ARGS}
    INSTALL_COMMAND ""
)

add_dependencies(
    mbutil
    mblog-android
)

# Always rebuild since files may have changed
ExternalProject_Add_Step(
    mbutil
    forcerebuild
    DEPENDEES update
    DEPENDERS configure
    ALWAYS 1
    COMMAND ""
)
