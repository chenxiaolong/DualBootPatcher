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
    else()
        set(MBP_SYSTEM_CMAKE_BUILD_TYPE Release)
    endif()

    # List of ExternalProject directories to run "make clean" in
    set(RECURSIVE_CLEAN_DIRS)

    # FIXME: Required until https://github.com/android-ndk/ndk/issues/222 is fixed
    set(ENV{ANDROID_NDK} "${ndk_path}")

    foreach(abi ${ANDROID_ABIS})
        set(INTERNAL_COMMON_OPTIONS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/android.toolchain.cmake
            #-DCMAKE_TOOLCHAIN_FILE=${ndk_path}/build/cmake/android.toolchain.cmake
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DANDROID_ABI=${abi}
            -DANDROID_STL=gnustl_static
            #-DANDROID_STL=c++_static
            -DMBP_BUILD_TYPE=${MBP_BUILD_TYPE}
            -DMBP_ENABLE_TESTS=OFF
            -DMBP_PREBUILTS_BINARY_DIR=${MBP_PREBUILTS_BINARY_DIR}
            -DMBP_SIGN_CONFIG_PATH=${MBP_SIGN_CONFIG_PATH}
            -DJAVA_KEYTOOL=${JAVA_KEYTOOL}
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
            -DMBP_BUILD_TARGET=android-app
        )

        set(INTERNAL_ANDROID_SYSTEM_OPTIONS
            ${INTERNAL_COMMON_OPTIONS}
            -DCMAKE_BUILD_TYPE=${MBP_SYSTEM_CMAKE_BUILD_TYPE}
            -DANDROID_PIE=OFF
            -DANDROID_PLATFORM=android-21
            -DMBP_BUILD_TARGET=android-system
        )

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
            INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install/strip
            VERBATIM
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

        ExternalProject_Get_Property(android-system_${abi} BINARY_DIR)
        list(APPEND RECURSIVE_CLEAN_DIRS ${BINARY_DIR})

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
                INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install/strip
                VERBATIM
            )

            ExternalProject_Get_Property(android-app_${abi} BINARY_DIR)
            list(APPEND RECURSIVE_CLEAN_DIRS ${BINARY_DIR})
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

    # Create clean target
    set(CLEAN_CONTENTS)
    foreach(_dir ${RECURSIVE_CLEAN_DIRS})
        string(CONCAT CLEAN_CONTENTS
               ${CLEAN_CONTENTS}
               "message(STATUS \"Cleaning ${_dir}\")\n"
               "execute_process(COMMAND \${CMAKE_COMMAND} --build \"${_dir}\" --target clean)\n")
    endforeach()

    file(WRITE
         ${CMAKE_CURRENT_BINARY_DIR}/clean-android.cmake
         "${CLEAN_CONTENTS}")

    add_custom_target(
        clean-android
        ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/clean-android.cmake
        VERBATIM
    )
endif()
