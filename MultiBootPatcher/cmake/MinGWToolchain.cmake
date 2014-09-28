# This file is UNUSED. If anyone wants to cross compile for Windows, this
# toolchain file should work, but official releases are compiled with
# Visual Studio 2013.

set(CMAKE_SYSTEM_NAME Windows)

set(COMPILER_PREFIX "i686-w64-mingw32")
#set(COMPILER_PREFIX "x86_64-w64-mingw32"

find_program(CMAKE_RC_COMPILER NAMES ${COMPILER_PREFIX}-windres)
find_program(CMAKE_C_COMPILER NAMES ${COMPILER_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)

set(CMAKE_FIND_ROOT_PATH /usr/${COMPILER_PREFIX}
                         /usr/${COMPILER_PREFIX}/sys-root/mingw)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
