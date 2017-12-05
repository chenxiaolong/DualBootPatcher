# Scripts
if(MBP_TOP_LEVEL_BUILD)
    # Copy scripts to binary dir so the signing process won't clobber the source
    # directory with .sig files
    file(COPY scripts DESTINATION .)

    set(scripts
        scripts/bb-wrapper.sh
    )

    set(source_files)
    set(target_files)

    foreach(file ${scripts})
        set(source_file "${CMAKE_CURRENT_BINARY_DIR}/${file}")
        set(target_file "${CMAKE_CURRENT_BINARY_DIR}/${file}.sig")

        list(APPEND source_files "${source_file}")
        list(APPEND target_files "${target_file}")

        get_filename_component(parent "${file}" DIRECTORY)

        install(
            FILES "${source_file}" "${target_file}"
            DESTINATION ${DATA_INSTALL_DIR}/${parent}/
            COMPONENT Libraries
        )
    endforeach()

    add_sign_files_target(
        sign_scripts
        ${source_files}
    )

    add_subdirectory(devices)
endif()
