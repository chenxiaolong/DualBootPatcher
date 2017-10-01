if(${MBP_BUILD_TARGET} STREQUAL desktop)
    qt5_add_resources(gui_resources_rcc resources.qrc)

    add_executable(
        gui
        WIN32
        main.cpp
        mainwindow.cpp
        ${gui_resources_rcc}
    )

    if(WIN32)
        target_sources(gui PRIVATE main.rc)
    endif()

    target_include_directories(
        gui
        PRIVATE
        .
    )

    target_link_libraries(
        gui
        PRIVATE
        interface.global.CXXVersion
        interface.mbcommon.dynamic-link
        Qt5::Core
        Qt5::Widgets
        mbpatcher-shared
    )

    if(${MBP_PORTABLE})
        set(data_dir "data")
    else()
        file(RELATIVE_PATH data_dir
             "/${BIN_INSTALL_DIR}" "/${DATA_INSTALL_DIR}")
    endif()

    target_compile_definitions(
        gui
        PRIVATE
        -DDATA_DIR="${data_dir}"
    )

    target_compile_definitions(
        gui
        PRIVATE
        -DQT_NO_CAST_FROM_ASCII
        -DQT_NO_CAST_TO_ASCII
        -DQT_NO_CAST_FROM_BYTEARRAY
    )

    if(NOT MSVC)
        target_compile_options(
            gui
            PRIVATE
            -Wno-zero-as-null-pointer-constant
        )
    endif()

    set_target_properties(
        gui
        PROPERTIES
        AUTOMOC ON
        OUTPUT_NAME DualBootPatcher
    )

    # Set rpath for portable build
    if(${MBP_PORTABLE})
        set_target_properties(
            gui
            PROPERTIES
            BUILD_WITH_INSTALL_RPATH OFF
            INSTALL_RPATH "\$ORIGIN/lib"
        )
    endif()

    install(
        TARGETS gui
        RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
        COMPONENT Applications
    )

    # Linux-specific files
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND NOT ${MBP_PORTABLE})
        install(
            FILES icon.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
            RENAME mbp.png
        )
        install(
            FILES mbp.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
        )
    endif()
endif()
