# Based on: https://github.com/WebKit/webkit/blob/5f13e8352a31b47bccbf6089fc9ac4a0d68f63b9/Source/cmake/WebKitCompilerFlags.cmake#L179
#
# References:
# - https://gitlab.kitware.com/cmake/cmake/issues/16291
# - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70129

# Add default system include directories to CMake's implicit includes. This
# fixes #include_next <...> with GCC 6 when cross-compiling.
macro(detect_system_include_dirs _lang _compiler _flags _result)
    file(WRITE "${CMAKE_BINARY_DIR}/CMakeFiles/dummy" "\n")
    separate_arguments(_build_flags UNIX_COMMAND "${_flags}")
    execute_process(
        COMMAND ${_compiler} ${_build_flags} -v -E -x ${_lang} -dD dummy
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/CMakeFiles
        OUTPUT_QUIET
        ERROR_VARIABLE _gcc_output
    )
    file(REMOVE "${CMAKE_BINARY_DIR}/CMakeFiles/dummy")

    if("${_gcc_output}" MATCHES "> search starts here[^\n]+\n *(.+) *\n *End of (search) list")
        set(${_result} ${CMAKE_MATCH_1})
        string(REPLACE "\n" " " ${_result} "${${_result}}")
        separate_arguments(${_result})
    endif()
endmacro()

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    detect_system_include_dirs(
        "c"
        "${CMAKE_C_COMPILER}"
        "${CMAKE_C_FLAGS}"
        system_include_dirs
    )
    list(APPEND CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES ${system_include_dirs})

    message(STATUS "Implicit C compiler include paths:")
    foreach(include_dir ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})
        message(STATUS "  ${include_dir}")
    endforeach()

    detect_system_include_dirs(
        "c++"
        "${CMAKE_CXX_COMPILER}"
        "${CMAKE_CXX_FLAGS}"
        system_include_dirs
    )
    list(APPEND CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES ${system_include_dirs})

    message(STATUS "Implicit C++ compiler include paths:")
    foreach(include_dir ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
        message(STATUS "  ${include_dir}")
    endforeach()
endif()
