if(NOT ${MBP_BUILD_TARGET} STREQUAL device)
    return()
endif()

add_executable(
    app
    main.cpp
    resources/resources.qrc
)

set_target_properties(
    app
    PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

target_compile_definitions(
    app
    PRIVATE
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

unix_link_executable_statically(app)

target_link_libraries(
    app
    PRIVATE
    interface.global.CXXVersion
    mbutil-static
    mbcommon-static
    Qt5::Core
    Qt5::Quick
    Qt5::modelsplugin
    Qt5::qmlsettingsplugin
    Qt5::qquicklayoutsplugin
    Qt5::qtquick2plugin
    Qt5::qtquickcontrols2universalstyleplugin
    Qt5::qtquickcontrols2plugin
    Qt5::qtquicktemplates2plugin
    Qt5::qlinuxfb
    Qt5::windowplugin
)

install(
    TARGETS app
    RUNTIME DESTINATION "${BIN_INSTALL_DIR}/"
    COMPONENT Applications
)
