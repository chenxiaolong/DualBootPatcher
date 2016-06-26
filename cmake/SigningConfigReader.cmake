cmake_minimum_required(VERSION 3.1)

function(read_signing_config out_prefix path)
    set(keystore)
    set(keystore_passphrase)
    set(key_alias)
    set(key_passphrase)

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
        if("${key}" STREQUAL "keystore")
            set(keystore "${value}")
        elseif("${key}" STREQUAL "keystore_passphrase")
            set(keystore_passphrase "${value}")
        elseif("${key}" STREQUAL "key_alias")
            set(key_alias "${value}")
        elseif("${key}" STREQUAL "key_passphrase")
            set(key_passphrase "${value}")
        else()
            message(FATAL_ERROR "Invalid key in signing config: ${key}")
        endif()
    endforeach()

    if(NOT keystore)
        message(FATAL_ERROR "Missing keystore path in: ${path}")
    endif()
    if(NOT keystore_passphrase)
        message(FATAL_ERROR "Missing keystore passphrase in: ${path}")
    endif()
    if(NOT key_alias)
        message(FATAL_ERROR "Missing key alias in: ${path}")
    endif()
    if(NOT key_passphrase)
        message(FATAL_ERROR "Missing key passphrase in: ${path}")
    endif()

    set("${out_prefix}KEYSTORE_PATH"
        "${keystore}" PARENT_SCOPE)
    set("${out_prefix}KEYSTORE_PASSPHRASE"
        "${keystore_passphrase}" PARENT_SCOPE)
    set("${out_prefix}KEY_ALIAS"
        "${key_alias}" PARENT_SCOPE)
    set("${out_prefix}KEY_PASSPHRASE"
        "${key_passphrase}" PARENT_SCOPE)
endfunction()
