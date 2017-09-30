add_library(
    mbbootui-gui
    STATIC
    action.cpp
    animation.cpp
    blanktimer.cpp
    button.cpp
    checkbox.cpp
    console.cpp
    fileselector.cpp
    fill.cpp
    gui.cpp
    hardwarekeyboard.cpp
    image.cpp
    input.cpp
    keyboard.cpp
    listbox.cpp
    monotonic_cond.cpp
    mousecursor.cpp
    object.cpp
    pages.cpp
    patternpassword.cpp
    progressbar.cpp
    resources.cpp
    scrolllist.cpp
    slider.cpp
    slidervalue.cpp
    terminal.cpp
    textbox.cpp
    text.cpp
    twmsg.cpp
)

target_include_directories(
    mbbootui-gui
    PRIVATE
    ..
    ${CMAKE_SOURCE_DIR}/external/flatbuffers/include
)

target_compile_options(
    mbbootui-gui
    PRIVATE
    -fno-strict-aliasing
    -Wno-sign-compare
    -Wno-format
    -Wno-undefined-bool-conversion
    -Werror=unused-function
    -Werror=unused-parameter
    -Werror=unused-variable
)

# terminal.cpp has a GNU-ism: uses "\e" instead of "\033" or "\x1b"
set_source_files_properties(
    terminal.cpp
    PROPERTIES
    COMPILE_FLAGS -no-pedantic
)

# Uncomment to enable event logging
#target_compile_definitions(
#    mbbootui-gui
#    PRIVATE
#    -D_EVENT_LOGGING
#)

target_link_libraries(
    mbbootui-gui
    PRIVATE
    interface.global.CXXVersion
    mbbootui-minui
    mbbootui-minzip
    mbdevice-static
    mblog-static
    mbutil-static
    AndroidSystemCore::Cutils
)
