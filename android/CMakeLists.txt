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
        set(cmake_options
            -DCMAKE_BUILD_TYPE=${MBP_SYSTEM_CMAKE_BUILD_TYPE}
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/android.toolchain.cmake
            #-DCMAKE_TOOLCHAIN_FILE=${ndk_path}/build/cmake/android.toolchain.cmake
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DANDROID_ABI=${abi}
            -DANDROID_PIE=OFF
            -DANDROID_PLATFORM=android-28
            -DANDROID_STL=c++_static
            -DMBP_BUILD_TARGET=android-system
            -DMBP_BUILD_TYPE=${MBP_BUILD_TYPE}
            -DMBP_ENABLE_TESTS=${MBP_ENABLE_TESTS}
            -DMBP_PREBUILTS_BINARY_DIR=${MBP_PREBUILTS_BINARY_DIR}
            -DMBP_SIGN_CONFIG_PATH=${MBP_SIGN_CONFIG_PATH}
            -DJAVA_KEYTOOL=${JAVA_KEYTOOL}
            -DMBSYSTRACE_HEADERSGEN_COMMAND=${MBSYSTRACE_HEADERSGEN_COMMAND}
            -DSCHEMAS2CPP_COMMAND=${SCHEMAS2CPP_COMMAND}
        )

        if(MBP_ANDROID_ENABLE_CCACHE)
            list(APPEND cmake_options
                 "-DANDROID_CCACHE=${MBP_CCACHE_PATH}")
        endif()

        if(MBP_CI_VERSION)
            list(APPEND cmake_options
                 "-DMBP_CI_VERSION=${MBP_CI_VERSION}")
        endif()

        if(MBP_ENABLE_QEMU)
            android_abi_to_qemu_arch("${abi}" qemu_arch)

            list(APPEND cmake_options
                 "-DMBP_ENABLE_QEMU=${MBP_ENABLE_QEMU}"
                 "-DQEMU_SYSTEM_COMMAND=${QEMU_SYSTEM_${qemu_arch}}")
        endif()

        message(STATUS "Recursive CMake build for Android system components (${abi}):")
        foreach(_option ${cmake_options})
            message(STATUS "  ${_option}")
        endforeach()

        # Always build system components
        ExternalProject_Add(
            android-system_${abi}
            SOURCE_DIR ${CMAKE_SOURCE_DIR}
            INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/result
            CMAKE_ARGS ${cmake_options}
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
                ${CMAKE_CURRENT_BINARY_DIR}/result/bin/${abi}/app
                ${ARCHIVE_TEMP_DIR}/exec

            # Copy info.prop
            COMMAND ${CMAKE_COMMAND}
                 -E copy
                 ${CMAKE_BINARY_DIR}/app/info.prop
                 ${ARCHIVE_TEMP_DIR}/info.prop

            # Create archive
            COMMAND ${CMAKE_COMMAND}
                -E tar cvf ${ARCHIVE_OUTPUT} --format=zip --
                exec
                #theme
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
endif()
