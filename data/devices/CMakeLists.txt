if(NOT MBP_TOP_LEVEL_BUILD)
    return()
endif()

set(files
    ark.yml
    asus.yml
    dexp.yml
    google.yml
    huawei.yml
    jiayu.yml
    lenovo.yml
    lg.yml
    motorola.yml
    nexus.yml
    oneplus.yml
    pantech.yml
    samsung/00_galaxy_s.yml
    samsung/01_galaxy_note.yml
    samsung/02_galaxy_j.yml
    samsung/03_galaxy_ace.yml
    samsung/04_galaxy_mega.yml
    samsung/05_galaxy_tab_tablet.yml
    samsung/06_galaxy_note_tablet.yml
    samsung/07_other.yml
    sony.yml
    wileyfox.yml
    xiaomi.yml
)

set(targets)

foreach(file ${files})
    set(source_file "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
    set(target_file "${CMAKE_CURRENT_BINARY_DIR}/${file}")
    string(REGEX REPLACE "\\.yml$" ".json" target_file "${target_file}")
    get_filename_component(target_dir "${target_file}" DIRECTORY)
    list(APPEND targets "${target_file}")

    add_custom_command(
        OUTPUT "${target_file}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${target_dir}"
        COMMAND "${YAML2JSON_COMMAND}" "${source_file}" -o "${target_file}"
        MAIN_DEPENDENCY "${source_file}"
        DEPENDS hosttools
        COMMENT "Converting device definition yaml to json: ${file}"
        VERBATIM
    )

    get_filename_component(parent "${file}" DIRECTORY)

    install(
        FILES "${target_file}"
        DESTINATION "${DATA_INSTALL_DIR}/devices/${parent}/"
        COMPONENT Libraries
    )
endforeach()

add_custom_target(
    run_yaml2json
    ALL
    DEPENDS ${targets}
)
