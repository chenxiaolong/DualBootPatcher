include(ExternalProject)

# Generate Android.mk
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.static.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.static.mk
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android.static.include.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/Android.static.include.mk
    @ONLY
)

ExternalProject_Add(
    mbutil-static-android
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out/
    BUILD_COMMAND ${NDK_BUILD}
                  -C <SOURCE_DIR>
                  NDK_PROJECT_PATH=.
                  NDK_APPLICATION_MK=Application.mk
                  NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out
                  NDK_OUT=<TMP_DIR>
                  APP_BUILD_SCRIPT=${CMAKE_CURRENT_BINARY_DIR}/Android.static.mk
                  ${NDK_ARGS}
    INSTALL_COMMAND ""
)

add_dependencies(
    mbutil-static-android
    mblog-static-android
)

# Always rebuild since files may have changed
ExternalProject_Add_Step(
    mbutil-static-android
    forcerebuild
    DEPENDEES update
    DEPENDERS configure
    ALWAYS 1
    COMMAND ""
)
