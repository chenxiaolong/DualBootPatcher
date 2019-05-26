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

function(get_compiler_command target_arch out_var)
    if(ANDROID)
        # On Android, we can't simply append eg. -m32 to CMAKE_CXX_FLAGS because
        # the NDK uses -target and -isystem to differentiate between
        # architectures.
        set(extra_args)

        if("${target_arch}" STREQUAL x86_64)
            set(target_triple x86_64-none-linux-android)
            set(header_triple x86_64-linux-android)
        elseif("${target_arch}" STREQUAL x86)
            set(target_triple i686-none-linux-android)
            set(header_triple i686-linux-android)
        elseif("${target_arch}" STREQUAL x32)
            set(target_triple x86_64-none-linux-android)
            set(header_triple x86_64-linux-android)
            set(extra_args -mx32)
        elseif("${target_arch}" STREQUAL aarch64)
            set(target_triple aarch64-none-linux-android)
            set(header_triple aarch64-linux-android)
        elseif("${target_arch}" STREQUAL arm)
            set(target_triple armv7-none-linux-androideabi)
            set(header_triple arm-linux-androideabi)
        endif()

        set("${out_var}"
            "${CMAKE_CXX_COMPILER}"
            --sysroot "${CMAKE_SYSROOT}"
            -target "${target_triple}${ANDROID_PLATFORM_LEVEL}"
            -isystem "${CMAKE_SYSROOT}/usr/include/${header_triple}"
            ${extra_args}
            PARENT_SCOPE
        )
    else()
        # On non-Android, assume that -m32, etc. can be used
        set(extra_args)

        if(NOT "${arch}" STREQUAL "${target_arch}")
            if("${target_arch}" STREQUAL x86 OR "${target_arch}" STREQUAL arm)
                set(extra_args -m32)
            elseif("${target_arch}" STREQUAL x32)
                set(extra_args -mx32)
            endif()
        endif()

        separate_arguments(flags UNIX_COMMAND "${CMAKE_CXX_FLAGS}")

        set("${out_var}"
            "${CMAKE_CXX_COMPILER}"
            ${flags}
            ${extra_args}
            PARENT_SCOPE
        )
    endif()
endfunction()

function(add_signals_target)
    set(target_file "${CMAKE_CURRENT_BINARY_DIR}/include/signals_list.h")
    get_compiler_command("${arch}" compiler_cmd)

    add_custom_command(
        OUTPUT "${target_file}"
        COMMAND "${MBSYSTRACE_HEADERSGEN_COMMAND}"
            -t signals
            -o "${target_file}"
            --
            ${compiler_cmd}
        DEPENDS ${headersgen_deps}
        COMMENT "Generating signals list"
        VERBATIM
    )

    set(generated_headers ${generated_headers} "${target_file}" PARENT_SCOPE)
endfunction()

function(add_syscalls_target target_arch)
    set(target_file "${CMAKE_CURRENT_BINARY_DIR}/include/syscalls_list.${target_arch}.h")
    get_compiler_command("${target_arch}" compiler_cmd)

    add_custom_command(
        OUTPUT "${target_file}"
        COMMAND "${MBSYSTRACE_HEADERSGEN_COMMAND}"
            -t syscalls
            -o "${target_file}"
            --
            ${compiler_cmd}
        DEPENDS ${headersgen_deps}
        COMMENT "Generating syscalls list for ${target_arch}"
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

# Generated signals header
add_signals_target()

# Generate syscalls header for host ABI
add_syscalls_target("${arch}")

# Generate syscalls headers for compatible ABIs
if("${arch}" STREQUAL x86_64)
    add_syscalls_target(x86)
    add_syscalls_target(x32)
elseif("${arch}" STREQUAL aarch64)
    add_syscalls_target(arm)
endif()

set(variants)

if(MBP_TARGET_HAS_BUILDS)
    list(APPEND variants static)
endif()
# No shared version

# Build libraries
foreach(variant ${variants})
    set(lib_target mbsystrace-${variant})
    string(TOUPPER ${variant} uvariant)

    # Build library
    add_library(
        ${lib_target}
        ${uvariant}
        src/event.cpp
        src/procfs.cpp
        src/registers.cpp
        src/signals.cpp
        src/signals_list.cpp
        src/syscalls.cpp
        src/tracee.cpp
        src/tracee/injection.cpp
        src/tracee/memory.cpp
        src/tracer.cpp
        src/tracer/mainloop.cpp
        src/arch/${arch}/arch.cpp
        src/arch/${arch}/syscalls_list.cpp
        ${generated_headers}
    )

    # Includes
    target_include_directories(
        ${lib_target}
        PUBLIC
        include
        include/arch/${arch}
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/include
    )

    # Only build static library if needed
    if(${variant} STREQUAL static)
        set_target_properties(${lib_target} PROPERTIES EXCLUDE_FROM_ALL 1)
    endif()

    # Set library name
    set_target_properties(${lib_target} PROPERTIES OUTPUT_NAME mbsystrace)

    # Link dependencies
    target_link_libraries(
        ${lib_target}
        PUBLIC
        mbcommon-${variant}
        OpenSSL::Crypto
        PRIVATE
        interface.global.CXXVersion
        interface.mbcommon.library
        $<$<STREQUAL:${variant},shared>:interface.mbcommon.dynamic-link>
    )

    # Install shared library
    if(${variant} STREQUAL shared)
        install(
            TARGETS ${lib_target}
            LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
            RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
            #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        )
    endif()
endforeach()

# Build tests
if(variants AND MBP_ENABLE_TESTS)
    # Build tests
    add_executable(
        mbsystrace_tests
        # Helpers
        tests/main.cpp
        # Tests
        tests/test_arch.cpp
        tests/test_attach_detach.cpp
        tests/test_execve.cpp
        tests/test_hooks.cpp
        tests/test_inject.cpp
        tests/test_memory.cpp
        tests/test_new_process.cpp
        tests/test_signals.cpp
        tests/test_syscalls.cpp
    )

    # Link dependencies
    target_link_libraries(
        mbsystrace_tests
        interface.global.CXXVersion
        mbsystrace-static
        gmock
        gmock_main
    )

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        target_compile_options(
            mbsystrace_tests
            PRIVATE
            -Wno-gnu-statement-expression
        )
    endif()

    if(${MBP_BUILD_TARGET} STREQUAL android-system)
        unix_link_executable_statically(mbsystrace_tests)
    endif()

    # Add to ctest
    add_gtest_test(mbsystrace_tests)
endif()
