if(WIN32)
    set(
        WINDOWS_SHORTCUT_SOURCES
        windows.cpp
        windows.rc
    )

    add_executable(
        DualBootPatcherGUI
        WIN32
        ${WINDOWS_SHORTCUT_SOURCES}
    )

    set_target_properties(
        DualBootPatcherGUI
        PROPERTIES
        COMPILE_FLAGS "-DGUI -DAPPLICATION=\\\"bin\\\\gui.exe\\\""
    )

    target_link_libraries(
        DualBootPatcherGUI
        shlwapi
    )

    # Install to root directory
    install(
        TARGETS DualBootPatcherGUI
        RUNTIME DESTINATION .
        COMPONENT Applications
    )
endif()