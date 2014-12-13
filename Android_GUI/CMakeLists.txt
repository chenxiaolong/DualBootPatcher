if(BUILD_ANDROID)
    include(ExternalProject)

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/build.gradle.in
        ${CMAKE_CURRENT_SOURCE_DIR}/build.gradle
        @ONLY
    )

    # Can't really build with CMake since it needs the CPack ZIP

#    if(ANDROID_DEBUG)
#        set(GRADLE_ARG "assembleDebug")
#    else()
#        set(GRADLE_ARG "assembleRelease")
#    endif()
#
#    ExternalProject_Add(
#        android-gui
#        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
#        CONFIGURE_COMMAND ""
#        BUILD_COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && ./gradlew ${GRADLE_ARG}
#        INSTALL_COMMAND ""
#    )
#
#    # Always rebuild since files may have changed
#    ExternalProject_Add_Step(
#        android-gui
#        forcerebuild
#        DEPENDEES update
#        DEPENDERS configure
#        ALWAYS 1
#        COMMAND ""
#    )
endif()
