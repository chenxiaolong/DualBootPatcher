if(NOT MBP_TOP_LEVEL_BUILD)
    return()
endif()

set(files
    360.yml
    acer.yml
    alcatel.yml
    amazon.yml
    archos.yml
    ark.yml
    asus.yml
    blu.yml
    bq.yml
    cagabi.yml
    cherrymobile.yml
    cmcc.yml
    coolpad.yml
    dexp.yml
    doogee.yml
    elephone.yml
    energy_sistem.yml
    ginzzu.yml
    gionee.yml
    google.yml
    gree.yml
    htc.yml
    homtom.yml
    huawei.yml
    infinix.yml
    infocus.yml
    jiayu.yml
    lava.yml
    leeco.yml
    leagoo.yml
    lenovo.yml
    lg.yml
    linnex.yml
    micromax.yml
    meizu.yml
    motorola.yml
    nokia.yml
    nextbit.yml
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
    symphony.yml
    tecno.yml
    umi.yml
    walton.yml
    wiko.yml
    wileyfox.yml
    xiaomi/00_mi.yml
    xiaomi/01_mi_note.yml
    xiaomi/02_mi_max.yml
    xiaomi/03_redmi.yml
    xiaomi/04_redmi_note.yml
    xiaomi/05_other.yml
    yu.yml
    zoji.yml
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
