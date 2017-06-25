if(${MBP_BUILD_TARGET} STREQUAL desktop)
    set(CMAKE_AUTOMOC ON)

    set(GUI_SOURCES
        main.cpp
        mainwindow.cpp
    )

    if(WIN32)
        set(GUI_SOURCES
            ${GUI_SOURCES}
            main.rc
        )
    endif()

    set(GUI_PRIVATE_HEADERS
        mainwindow_p.h
    )

    set(GUI_HEADERS
        mainwindow.h
    )

    set(GUI_RESOURCES
        resources.qrc
    )

    qt5_add_resources(GUI_RESOURCES_RCC ${GUI_RESOURCES})

    add_executable(gui WIN32 ${GUI_SOURCES} ${GUI_RESOURCES_RCC})

    target_include_directories(
        gui
        PRIVATE
        .
    )

    target_link_libraries(
        gui
        PRIVATE
        Qt5::Core
        Qt5::Widgets
        mbp-shared
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
        -DMB_DYNAMIC_LINK
    )

    if(NOT MSVC)
        set_target_properties(
            gui
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    set_target_properties(gui
        PROPERTIES
        OUTPUT_NAME DualBootPatcher
    )

    # Set rpath for portable build
    if(${MBP_PORTABLE})
        set_target_properties(gui
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
