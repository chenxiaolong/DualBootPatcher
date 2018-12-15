include(CheckCXXSymbolExists)

add_subdirectory(headersgen)

if(NOT ${MBP_BUILD_TARGET} STREQUAL android-system
        AND NOT (${MBP_BUILD_TARGET} STREQUAL desktop AND UNIX))
    return()
endif()

if(NOT CMAKE_COMPILER_IS_GNUCXX
        AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(FATAL_ERROR "libmbsystrace requires gcc/clang")
endif()

if(${MBP_BUILD_TARGET} STREQUAL desktop)
    set(headersgen_deps hosttools)
else()
    set(headersgen_deps)
endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")

set(generated_headers)

function(add_signals_target)
    set(target_file "${CMAKE_CURRENT_BINARY_DIR}/include/signals_list.h")
    separate_arguments(flags UNIX_COMMAND "${CMAKE_CXX_FLAGS}")
    set(compiler_args "${CMAKE_CXX_COMPILER}" ${flags})

    add_custom_command(
        OUTPUT "${target_file}"
        COMMAND "${MBSYSTRACE_HEADERSGEN_COMMAND}"
            -t signals
            -o "${target_file}"
            --
            ${compiler_args}
        DEPENDS ${headersgen_deps}
        COMMENT "Generating signals list"
        VERBATIM
    )

    set(generated_headers ${generated_headers} "${target_file}" PARENT_SCOPE)
endfunction()

function(add_syscalls_target abi)
    set(target_file "${CMAKE_CURRENT_BINARY_DIR}/include/syscalls_list.${abi}.h")
    separate_arguments(flags UNIX_COMMAND "${CMAKE_CXX_FLAGS}")
    set(compiler_args "${CMAKE_CXX_COMPILER}" ${flags} ${ARGN})

    add_custom_command(
        OUTPUT "${target_file}"
        COMMAND "${MBSYSTRACE_HEADERSGEN_COMMAND}"
            -t syscalls
            -o "${target_file}"
            --
            ${compiler_args}
        DEPENDS ${headersgen_deps}
        COMMENT "Generating syscalls list for ${abi}"
        VERBATIM
    )

    set(generated_headers ${generated_headers} "${target_file}" PARENT_SCOPE)
endfunction()

function(get_arch out_var)
    # <arch name>::<compiler macro>
    set(known_arches
        x86_64::__x86_64__
        x86::__i386__
        aarch64::__aarch64__
        arm::__arm__
    )
    set(checked_arches)

    foreach(known_arch ${known_arches})
        if("${known_arch}" MATCHES "^(.+)::(.+)$")
            set(arch_name "${CMAKE_MATCH_1}")
            set(arch_macro "${CMAKE_MATCH_2}")
            set(arch_var "ARCH_IS_${arch_name}")

            check_cxx_symbol_exists("${arch_macro}" "" "${arch_var}")
            if("${${arch_var}}")
                set("${out_var}" "${arch_name}" PARENT_SCOPE)
                return()
            endif()

            set(checked_arches ${checked_arches} "${arch_name}")
        endif()
    endforeach()

    message(FATAL_ERROR "Failed to detect arch in ${checked_arches}")
endfunction()

get_arch(arch)

# Generated targets
add_signals_target()

if("${arch}" STREQUAL x86_64)
    add_syscalls_target(x86_64)
    add_syscalls_target(x86 -m32)
    add_syscalls_target(x32 -mx32)
elseif("${arch}" STREQUAL x86)
    add_syscalls_target(x86)
elseif("${arch}" STREQUAL aarch64)
    add_syscalls_target(aarch64)
    add_syscalls_target(arm -m32)
elseif("${arch}" STREQUAL arm)
    add_syscalls_target(arm)
else()
    message(FATAL_ERROR "Unsupported architecture for libmbsystrace")
endif()
