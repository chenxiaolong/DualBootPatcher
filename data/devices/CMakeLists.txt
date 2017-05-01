if(NOT MBP_TOP_LEVEL_BUILD)
    return()
endif()

set(files
    ark.yml
    asus.yml
    coolpad.yml
    dexp.yml
    elephone.yml
    google.yml
    huawei.yml
    infinix.yml
    jiayu.yml
    leeco.yml
    lenovo.yml
    lg.yml
    meizu.yml
    motorola.yml
    nexus.yml
    nubia.yml
    oneplus.yml
    oppo.yml
    oukitel.yml
    pantech.yml
    quantum.yml
    samsung/00_galaxy_s.yml
    samsung/01_galaxy_note.yml
    samsung/02_galaxy_a_j.yml
    samsung/03_galaxy_ace.yml
    samsung/04_galaxy_mega.yml
    samsung/05_galaxy_tab_tablet.yml
    samsung/06_galaxy_note_tablet.yml
    samsung/07_other.yml
    sony.yml
    umi.yml
    wileyfox.yml
    xiaomi.yml
    yu.yml
    zte.yml
    zuk.yml
)

set(target_file "${CMAKE_CURRENT_BINARY_DIR}/devices.json")

add_custom_command(
    OUTPUT "${target_file}"
    COMMAND "${DEVICESGEN_COMMAND}"
        ${files}
        -o "${target_file}"
        #--styled
    DEPENDS hosttools ${files}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating device defition JSON file"
    VERBATIM
)

install(
    FILES "${target_file}"
    DESTINATION "${DATA_INSTALL_DIR}/"
    COMPONENT Libraries
)

add_custom_target(
    run_devicesgen
    ALL
    DEPENDS ${target_file}
)
