if(${MBP_BUILD_TARGET} STREQUAL android)
    if(${MBP_BUILD_TYPE} STREQUAL release)
        set(_RELEASE_SIGN_CONFIG_PATH "${MBP_SIGN_CONFIG_PATH}")
    elseif(${MBP_BUILD_TYPE} STREQUAL debug)
        set(_DEBUG_SIGN_CONFIG_PATH "${MBP_SIGN_CONFIG_PATH}")
    elseif(${MBP_BUILD_TYPE} STREQUAL ci)
        set(_CI_SIGN_CONFIG_PATH "${MBP_SIGN_CONFIG_PATH}")
    endif()

    include("${CMAKE_SOURCE_DIR}/cmake/external/GetGitRevisionDescription.cmake")
    get_git_head_revision(refspec _GIT_SHA)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/build.gradle.in
        ${CMAKE_CURRENT_SOURCE_DIR}/build.gradle
        @ONLY
    )

    if(WIN32)
        set(GRADLEW_CMD ".\\gradlew.bat")
    else()
        set(GRADLEW_CMD "./gradlew")
    endif()

    if(${MBP_BUILD_TYPE} STREQUAL release)
        set(GRADLEW_ARG "assembleRelease")
    elseif(${MBP_BUILD_TYPE} STREQUAL debug)
        set(GRADLEW_ARG "assembleDebug")
    elseif(${MBP_BUILD_TYPE} STREQUAL ci)
        set(GRADLEW_ARG "assembleCi")
    endif()

    add_custom_target(
        apk ${GRADLEW_CMD} -Pandroid.builder.sdkDownload=false ${GRADLEW_ARG}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/build.gradle
                #${CMAKE_BINARY_DIR}/assets/data-${MBP_VERSION}.tar.xz
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )

    add_custom_target(
        apk-install adb install -r build/outputs/apk/${MBP_BUILD_TYPE}/Android_GUI-${MBP_BUILD_TYPE}.apk
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )
    add_dependencies(apk-install apk)
endif()
