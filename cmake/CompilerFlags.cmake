if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

    # Enable warnings
    add_compile_options(-Wall -Wextra -Wpedantic -pedantic)

    #add_compile_options(-Werror)

    # Except for "/*" within comment errors (present in doxygen blocks)
    add_compile_options(-Wno-error=comment)

    # outcome.hpp
    add_compile_options(-Wno-parentheses)

    # Enable PIC
    set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

    # Enable stack-protector
    add_compile_options(-fstack-protector-strong --param ssp-buffer-size=4)

    # Enable _FORTIFY_SOURCE for release builds
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_FORTIFY_SOURCE=2")

    if(ANDROID)
        add_compile_options(-fno-exceptions -fno-rtti)

        # -Wunknown-attributes - malloc.h
        # -Wzero-length-array - asm-generic/siginfo.h
        add_compile_options(-Wno-unknown-attributes -Wno-zero-length-array)

        # Prevent libarchive's android_lf.h from being included. It #defines
        # a bunch of things like `#define open open64`, which breaks the build
        # process. The header is only needed for building libarchive anyway.
        add_definitions(-DARCHIVE_ANDROID_LF_H_INCLUDED)
    endif()

    if(WIN32)
        # Don't warn for valid msprintf specifiers
        add_compile_options(-Wno-pedantic-ms-format)

        # Target Vista and up
        add_definitions(-D_WIN32_WINNT=0x0600)

        # Enable ASLR
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--dynamicbase")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--dynamicbase")

        # Enable DEP
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--nxcompat")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--nxcompat")

        # libssp is required if stack-protector is enabled
        set(CMAKE_C_CREATE_SHARED_LIBRARY "${CMAKE_C_CREATE_SHARED_LIBRARY} -lssp")
        set(CMAKE_CXX_CREATE_SHARED_LIBRARY "${CMAKE_CXX_CREATE_SHARED_LIBRARY} -lssp")
        SET(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE} -lssp")
        SET(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -lssp")
    else()
        # Visibility
        add_compile_options(-fvisibility=hidden)

        # Use DT_RPATH instead of DT_RUNPATH because the latter is not transitive
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")

        # Enable PIE
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie")

        # Disable executable stack
        add_compile_options(-Wa,--noexecstack)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,noexecstack")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,noexecstack")

        # Enable full relro
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro -Wl,-z,now")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,relro -Wl,-z,now")

        # Does not work on Windows:
        # https://sourceware.org/bugzilla/show_bug.cgi?id=11539
        add_compile_options(-ffunction-sections -fdata-sections)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")
    endif()

    # GCC-specific warnings
    if(CMAKE_COMPILER_IS_GNUCXX)
        add_compile_options(
            -Walloca
            $<$<COMPILE_LANGUAGE:C>:-Wbad-function-cast>
            -Wcast-align # TODO Maybe enable =strict
            -Wcast-qual
            -Wconversion
            -Wdate-time
            -Wdouble-promotion
            -Wduplicated-branches
            -Wduplicated-cond
            -Wenum-compare # C only
            #-Wextra-semi
            -Wfloat-equal
            -Wformat=2
            -Wformat-overflow=2
            -Wformat-truncation=2
            $<$<COMPILE_LANGUAGE:C>:-Wjump-misses-init>
            -Wlogical-op
            -Wmissing-declarations
            -Wmissing-include-dirs
            $<$<COMPILE_LANGUAGE:C>:-Wmissing-prototypes>
            $<$<COMPILE_LANGUAGE:C>:-Wnested-externs>
            -Wnull-dereference
            $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
            $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
            -Wredundant-decls
            -Wrestrict
            #-Wshadow # Disabled because this doesn't ignore member initializers
            -Wsign-conversion
            -Wtrampolines
            #-Wundef # Too many libraries to #if for undefined macros
            #-Wuseless-cast # Disabled because of mb::Flags
            -Wwrite-strings # C only
            $<$<COMPILE_LANGUAGE:CXX>:-Wzero-as-null-pointer-constant>
        )
    endif()

    # Clang-specific warnings
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options(
            -Wabstract-vbase-init
            -Warray-bounds-pointer-arithmetic
            -Wassign-enum
            -Wbad-function-cast
            -Wc++11-extensions
            -Wc++14-extensions
            -Wcast-align
            -Wcast-qual
            -Wclass-varargs
            -Wcomma
            -Wconditional-uninitialized
            -Wconsumed
            -Wconversion
            -Wdate-time
            -Wdocumentation
            -Wdocumentation-pedantic
            -Wdouble-promotion
            -Wduplicate-enum
            -Wduplicate-method-arg
            -Wduplicate-method-match
            #-Wexit-time-destructors # We have global variables that are classes
            -Wfloat-equal
            -Wformat=2
            -Wheader-hygiene
            -Widiomatic-parentheses
            -Wimplicit-fallthrough
            -Wloop-analysis
            -Wmethod-signatures
            -Wmissing-noreturn
            -Wmissing-prototypes
            -Wmissing-variable-declarations
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wover-aligned
            #-Wreserved-id-macro # Too many libraries have macros with an underscore prefix
            -Wshadow-all
            -Wstrict-prototypes
            -Wstrict-selector-match
            -Wsuper-class-method-mismatch
            -Wtautological-compare
            -Wthread-safety
            #-Wundef # Too many libraries to #if for undefined macros
            -Wundefined-func-template
            -Wundefined-reinterpret-cast
            -Wunreachable-code-aggressive
        )

        # Disable warnings
        add_compile_options(
            -Wno-documentation-unknown-command # \cond is not understood
            -Wno-missing-noreturn # Caused by disabling exceptions in Outcome
            -Wno-shadow-field-in-constructor
        )
    endif()
endif()

# Remove some warnings caused by the BoringSSL headers on Android
if(ANDROID AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-Wno-gnu-anonymous-struct -Wno-nested-anon-types)
endif()

add_library(interface.global.CVersion INTERFACE)
add_library(interface.global.CXXVersion INTERFACE)

if(NOT MSVC)
    # Target C11
    target_compile_features(
        interface.global.CVersion
        INTERFACE
        c_std_11
    )

    # Target C++17
    target_compile_features(
        interface.global.CXXVersion
        INTERFACE
        cxx_std_17
    )
endif()

function(unix_link_executable_statically first_target)
    if(UNIX)
        foreach(target "${first_target}" ${ARGN})
            set_property(
                TARGET "${target}"
                APPEND_STRING
                PROPERTY LINK_FLAGS " -static"
            )
            set_target_properties(
                "${target}"
                PROPERTIES
                LINK_SEARCH_START_STATIC ON
            )
        endforeach()
    endif()
endfunction()
