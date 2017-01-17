# cmake 3.7 has a bug where add_custom_command() appends the target name as an
# argument if running an imported command. This breaks cross compiling for
# Windows because qt5_add_resources() calls add_custom_command() with the
# Qt5::rcc target.
#
# We'll work around this by unsetting CMAKE_CROSSCOMPILING_EMULATOR.
#
# See: https://gitlab.kitware.com/cmake/cmake/issues/16288
if(CMAKE_VERSION VERSION_LESS 3.8 AND CMAKE_CROSSCOMPILING_EMULATOR)
    message(WARNING "Applying hack for CMake bug 16288")
    set(emulator_bak "${CMAKE_CROSSCOMPILING_EMULATOR}")
    unset(CMAKE_CROSSCOMPILING_EMULATOR)
endif()

find_package(Qt5Core 5.3 REQUIRED)
find_package(Qt5Widgets 5.3 REQUIRED)

if(emulator_bak)
    set(CMAKE_CROSSCOMPILING_EMULATOR "${emulator_bak}")
endif()
