cmake_minimum_required(VERSION 3.1)

function(read_signing_config out_prefix path)
    set(secret_key_path)
    set(secret_key_passphrase)
    set(public_key_path)

    file(STRINGS ${path} contents)
    foreach(line ${contents})
        # Skip comments
        if(line MATCHES "^[ \t]*#")
            continue()
        endif()

        # Ensure <key>=<value> format
        string(REGEX MATCH "(^[^=]+)=" left ${line})
        if(NOT left)
            message(FATAL_ERROR "Invalid line in signing config: ${line}")
        endif()
        string(REPLACE "=" "" key ${left})
        string(REPLACE "${left}" "" value ${line})

        # Parse key/value pairs
        if("${key}" STREQUAL "secret_key_path")
            set(secret_key_path "${value}")
        elseif("${key}" STREQUAL "secret_key_passphrase")
            set(secret_key_passphrase "${value}")
        elseif("${key}" STREQUAL "public_key_path")
            set(public_key_path "${value}")
        else()
            message(FATAL_ERROR "Invalid key in signing config: ${key}")
        endif()
    endforeach()

    if(NOT secret_key_path)
        message(FATAL_ERROR "Missing secret key path in: ${path}")
    endif()
    if(NOT secret_key_passphrase)
        message(FATAL_ERROR "Missing secret key passphrase in: ${path}")
    endif()
    if(NOT public_key_path)
        message(FATAL_ERROR "Missing public key path in: ${path}")
    endif()

    set("${out_prefix}SECRET_KEY_PATH"
        "${secret_key_path}" PARENT_SCOPE)
    set("${out_prefix}SECRET_KEY_PASSPHRASE"
        "${secret_key_passphrase}" PARENT_SCOPE)
    set("${out_prefix}PUBLIC_KEY_PATH"
        "${public_key_path}" PARENT_SCOPE)
endfunction()
