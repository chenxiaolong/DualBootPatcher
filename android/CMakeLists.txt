if(MBP_TOP_LEVEL_BUILD)
    include(ExternalProject)

    if(DEFINED ANDROID_NDK_HOME AND EXISTS "${ANDROID_NDK_HOME}")
        set(ndk_path "${ANDROID_NDK_HOME}")
    elseif(DEFINED ANDROID_NDK AND EXISTS "${ANDROID_NDK}")
        set(ndk_path "${ANDROID_NDK}")
    elseif(DEFINED ENV{ANDROID_NDK_HOME} AND EXISTS "$ENV{ANDROID_NDK_HOME}")
        set(ndk_path "$ENV{ANDROID_NDK_HOME}")
    elseif(DEFINED ENV{ANDROID_NDK} AND EXISTS "$ENV{ANDROID_NDK}")
        set(ndk_path "$ENV{ANDROID_NDK}")
    else()
        message(FATAL_ERROR "Could not find Android NDK")
    endif()

    set(ANDROID_ABIS armeabi-v7a arm64-v8a x86 x86_64)

    # Always make release builds for the system components unless explicitly
    # told not to since a debug build of mbtool is too large to fit in some
    # devices' ramdisks.
    if(${MBP_SYSTEM_BUILD_TYPE} STREQUAL debug)
        set(MBP_SYSTEM_CMAKE_BUILD_TYPE Debug)
        set(system_install_target install)
    else()
        set(MBP_SYSTEM_CMAKE_BUILD_TYPE Release)
        set(system_install_target install/strip)
    endif()

    if(${MBP_BUILD_TARGET} STREQUAL android)
        if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
            set(app_install_target install)
        else()
            set(app_install_target install/strip)
        endif()
    endif()

    function(create_project_wrapper_script project)
        ExternalProject_Get_Property("${project}" BINARY_DIR)

        # Create wrapper script for running ExternalProject targets from the
        # top-level build
        string(CONCAT script
               "message(STATUS \"Running \${TARGET} in ${BINARY_DIR}\")\n"
               "execute_process(COMMAND \${CMAKE_COMMAND} --build \"${BINARY_DIR}\" --target \"\${TARGET}\")\n")

        file(WRITE
             "${CMAKE_CURRENT_BINARY_DIR}/run_target.${project}.cmake"
             "${script}")
    endfunction()

    function(expose_project_target project target)
        add_custom_target(
            "${project}_${target}"
            ${CMAKE_COMMAND}
                -D "TARGET=${target}"
                -P "${CMAKE_CURRENT_BINARY_DIR}/run_target.${project}.cmake"
            VERBATIM
            USES_TERMINAL
        )
    endfunction()

    # FIXME: Required until https://github.com/android-ndk/ndk/issues/222 is fixed
    set(ENV{ANDROID_NDK} "${ndk_path}")

    foreach(abi ${ANDROID_ABIS})
        set(INTERNAL_COMMON_OPTIONS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/android.toolchain.cmake
            #-DCMAKE_TOOLCHAIN_FILE=${ndk_path}/build/cmake/android.toolchain.cmake
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DANDROID_ABI=${abi}
            -DMBP_BUILD_TYPE=${MBP_BUILD_TYPE}
            -DMBP_ENABLE_TESTS=${MBP_ENABLE_TESTS}
            -DMBP_PREBUILTS_BINARY_DIR=${MBP_PREBUILTS_BINARY_DIR}
            -DMBP_SIGN_CONFIG_PATH=${MBP_SIGN_CONFIG_PATH}
            -DJAVA_KEYTOOL=${JAVA_KEYTOOL}
            -DSCHEMAS2CPP_COMMAND=${SCHEMAS2CPP_COMMAND}
        )

        if(MBP_ANDROID_ENABLE_CCACHE)
            list(APPEND INTERNAL_COMMON_OPTIONS
                 "-DANDROID_CCACHE=${MBP_CCACHE_PATH}")
        endif()

        if(MBP_CI_VERSION)
            list(APPEND INTERNAL_COMMON_OPTIONS
                 "-DMBP_CI_VERSION=${MBP_CI_VERSION}")
        endif()

        set(INTERNAL_ANDROID_APP_OPTIONS
            ${INTERNAL_COMMON_OPTIONS}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DANDROID_PIE=ON
            -DANDROID_PLATFORM=android-17
            -DANDROID_STL=c++_shared
            -DMBP_BUILD_TARGET=android-app
        )

        set(INTERNAL_ANDROID_SYSTEM_OPTIONS
            ${INTERNAL_COMMON_OPTIONS}
            -DCMAKE_BUILD_TYPE=${MBP_SYSTEM_CMAKE_BUILD_TYPE}
            -DANDROID_PIE=OFF
            -DANDROID_PLATFORM=android-28
            -DANDROID_STL=c++_static
            -DMBP_BUILD_TARGET=android-system
            -DMBSYSTRACE_HEADERSGEN_COMMAND=${MBSYSTRACE_HEADERSGEN_COMMAND}
        )

        if(MBP_ENABLE_QEMU)
            android_abi_to_qemu_arch("${abi}" qemu_arch)

            list(APPEND INTERNAL_ANDROID_SYSTEM_OPTIONS
                 "-DMBP_ENABLE_QEMU=${MBP_ENABLE_QEMU}"
                 "-DQEMU_SYSTEM_COMMAND=${QEMU_SYSTEM_${qemu_arch}}")
        endif()

        message(STATUS "Recursive CMake build for Android system components (${abi}):")
        foreach(_option ${INTERNAL_ANDROID_SYSTEM_OPTIONS})
            message(STATUS "  ${_option}")
        endforeach()

        # Always build system components
        ExternalProject_Add(
            android-system_${abi}
            SOURCE_DIR ${CMAKE_SOURCE_DIR}
            INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/result
            CMAKE_ARGS ${INTERNAL_ANDROID_SYSTEM_OPTIONS}
            BUILD_ALWAYS 1
            INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target ${system_install_target}
            VERBATIM
        )

        add_dependencies(
            android-system_${abi}
            hosttools
        )

        set(result_files
            ${CMAKE_CURRENT_BINARY_DIR}/result/bin/${abi}/mbtool
            ${CMAKE_CURRENT_BINARY_DIR}/result/bin/${abi}/mbtool_recovery
            ${CMAKE_CURRENT_BINARY_DIR}/result/bin/${abi}/odinupdater
            ${CMAKE_CURRENT_BINARY_DIR}/result/bin/${abi}/fuse-sparse
            ${CMAKE_CURRENT_BINARY_DIR}/result/bin/${abi}/file-contexts-tool
            ${CMAKE_CURRENT_BINARY_DIR}/result/bin/${abi}/fsck-wrapper)
        set(sig_files "")
        foreach(file ${result_files})
            list(APPEND sig_files "${file}.sig")
        endforeach()

        add_sign_files_target(
            sign_android-system_${abi}
            ${result_files}
        )
        add_dependencies(
            sign_android-system_${abi}
            android-system_${abi}
        )

        create_project_wrapper_script("android-system_${abi}")
        expose_project_target("android-system_${abi}" clean)

        if(MBP_ENABLE_QEMU)
            expose_project_target("android-system_${abi}" qemu-shell)
            expose_project_target("android-system_${abi}" qemu-tests)
        endif()

        install(
            FILES ${result_files} ${sig_files}
            DESTINATION ${DATA_INSTALL_DIR}/binaries/android/${abi}/
            COMPONENT Libraries
        )

        # Build app components only if we're targeting Android
        if(${MBP_BUILD_TARGET} STREQUAL android)
            message(STATUS "Recursive CMake build for Android app components (${abi}):")
            foreach(_option ${INTERNAL_ANDROID_APP_OPTIONS})
                message(STATUS "  ${_option}")
            endforeach()

            ExternalProject_Add(
                android-app_${abi}
                SOURCE_DIR ${CMAKE_SOURCE_DIR}
                INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/result
                CMAKE_ARGS ${INTERNAL_ANDROID_APP_OPTIONS}
                BUILD_ALWAYS 1
                INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target ${app_install_target}
                VERBATIM
            )

            add_dependencies(
                android-app_${abi}
                hosttools
            )

            create_project_wrapper_script("android-app_${abi}")
            expose_project_target("android-app_${abi}" clean)
        endif()

        set(ARCHIVE_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bootui/${abi}/bootui.zip)
        set(ARCHIVE_TEMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/bootui/${abi}/temp)

        add_custom_target(
            bootui-tempdir_${abi} ALL
            ${CMAKE_COMMAND} -E remove_directory ${ARCHIVE_TEMP_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${ARCHIVE_TEMP_DIR}
            COMMENT "Create boot UI temporary directory (${abi})"
            VERBATIM
        )

        add_custom_target(
            bootui-archive_${abi} ALL

            # Copy main binary
            ${CMAKE_COMMAND}
                -E copy
                ${CMAKE_CURRENT_BINARY_DIR}/result/bin/${abi}/mbbootui
                ${ARCHIVE_TEMP_DIR}/exec

            # Copy themes
            COMMAND ${CMAKE_COMMAND}
                -E copy_directory
                ${CMAKE_SOURCE_DIR}/mbbootui/theme
                ${ARCHIVE_TEMP_DIR}/theme

            # Copy info.prop
            COMMAND ${CMAKE_COMMAND}
                 -E copy
                 ${CMAKE_BINARY_DIR}/mbbootui/info.prop
                 ${ARCHIVE_TEMP_DIR}/info.prop

            # Create archive
            COMMAND ${CMAKE_COMMAND}
                -E tar cvf ${ARCHIVE_OUTPUT} --format=zip --
                exec
                theme
                info.prop

            # Delete temporary directory
            COMMAND ${CMAKE_COMMAND}
                -E remove_directory ${ARCHIVE_TEMP_DIR}

            WORKING_DIRECTORY ${ARCHIVE_TEMP_DIR}
            COMMENT "Create boot UI archive (${abi})"
            VERBATIM
        )

        add_dependencies(
            bootui-archive_${abi}
            bootui-tempdir_${abi}
            android-system_${abi}
        )

        add_sign_files_target(
            sign_bootui-archive_${abi}
            ${ARCHIVE_OUTPUT}
        )
        add_dependencies(
            sign_bootui-archive_${abi}
            bootui-archive_${abi}
        )

        install(
            FILES ${ARCHIVE_OUTPUT} ${ARCHIVE_OUTPUT}.sig
            DESTINATION ${DATA_INSTALL_DIR}/bootui/${abi}/
            COMPONENT Libraries
        )
    endforeach()
elseif(${MBP_BUILD_TARGET} STREQUAL android-app)
    # Install libc++. We do this in the android-app target because we have
    # access to the CMAKE_LINKER variable.
    set(stl_source "$ENV{ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ABI}/libc++_shared.so")
    set(stl_target "${CMAKE_CURRENT_BINARY_DIR}/libc++_shared.stripped.so")

    add_custom_command(
        OUTPUT "${stl_target}"
        COMMAND "${CMAKE_STRIP}" -s "${stl_source}" -o "${stl_target}"
        COMMENT "Strip libc++_shared library"
        VERBATIM
    )

    add_custom_target(
        strip_stl_shared
        ALL
        DEPENDS "${stl_target}"
    )

    install(
        FILES "${stl_target}"
        DESTINATION ${LIB_INSTALL_DIR}
        RENAME libc++_shared.so
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                    GROUP_EXECUTE             GROUP_READ
                    WORLD_EXECUTE             WORLD_READ
        COMPONENT Libraries
    )
endif()
