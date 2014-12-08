include(ExternalProject)
include(${CMAKE_SOURCE_DIR}/cmake/LogicalCoreCount.cmake)

# To update jsoncpp, use get-jsoncpp.sh or uncomment the lines below
# cmake's external project feature is not used by default for jsoncpp
# since it requires internet access during the build process

#ExternalProject_Add(
#    jsoncpp
#    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp
#    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
#    GIT_TAG 60f778b9fcd1c59908293a6145dffd369d4f6daf
#    CONFIGURE_COMMAND cd <SOURCE_DIR> && python2 amalgamate.py
#    BUILD_COMMAND ""
#    INSTALL_COMMAND ""
#)

ExternalProject_Add(
    syncdaemon
#    DEPENDS jsoncpp
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/out/
    BUILD_COMMAND ${NDK_BUILD}
                  -C <SOURCE_DIR>
                  -j${PROCESSOR_COUNT}
                  NDK_PROJECT_PATH=.
                  NDK_APPLICATION_MK=Application.mk
                  NDK_APP_LIBS_OUT=${CMAKE_CURRENT_BINARY_DIR}/out
                  NDK_OUT=<TMP_DIR>
                  SYNCDAEMON_VERSION=${MBP_VERSION}
                  ${NDK_ARGS}
    INSTALL_COMMAND ""
)

# Always rebuild since files may have changed
ExternalProject_Add_Step(
    syncdaemon
    forcerebuild
    DEPENDEES update
    DEPENDERS configure
    ALWAYS 1
    COMMAND ""
)

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out/
    DESTINATION ${DATA_INSTALL_DIR}/binaries/android
    COMPONENT Libraries
)

# Static analyzer
# cppcheck \
#     --enable=all \
#     --inconclusive \
#     common.cpp \
#     configfile.cpp \
#     syncdaemon.cpp \
#     -Ijsoncpp-dist \
#     --force

# On-device valgrind
# /data/local/Inst/bin/valgrind
#     -v
#     --leak-check=full
#     --show-leak-kinds=all
#     --track-origins=yes
#     ./a.out

# TODO: Apply UPX
