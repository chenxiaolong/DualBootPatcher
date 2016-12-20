# We don't need the keys for building the host tools
if(${MBP_BUILD_TARGET} STREQUAL hosttools)
    return()
endif()

# Find Java's keytool
find_program(JAVA_KEYTOOL NAMES keytool)
if(NOT JAVA_KEYTOOL)
    message(FATAL_ERROR "The java 'keytool' program was not found")
endif()

# Signing config
set(MBP_SIGN_CONFIG_PATH "" CACHE FILEPATH "Signing configuration file path")

# Create debug keystore if it doesn't exist
if(${MBP_BUILD_TYPE} STREQUAL debug AND "${MBP_SIGN_CONFIG_PATH}" STREQUAL "")
    message(WARNING "MBP_SIGN_CONFIG_PATH wasn't specified, but because this "
            "is a debug build, a signing config will be generated for the "
            "default Android SDK debug signing key.")

    # Only use Windows path when natively compiling, not cross-compiling
    #if(WIN32)
    if(CMAKE_HOST_WIN32)
        set(DOT_ANDROID_DIR "$ENV{USERPROFILE}\\.android")
        set(DEBUG_KEYSTORE "${DOT_ANDROID_DIR}\\debug.keystore")
    else()
        set(DOT_ANDROID_DIR "$ENV{HOME}/.android")
        set(DEBUG_KEYSTORE "${DOT_ANDROID_DIR}/debug.keystore")
    endif()

    if(NOT EXISTS "${DOT_ANDROID_DIR}")
        file(MAKE_DIRECTORY "${DOT_ANDROID_DIR}")
    endif()

    if(NOT EXISTS "${DEBUG_KEYSTORE}")
        message(WARNING "Building in debug mode, but debug keystore does not"
                        " exist. Creating one ...")
        execute_process(
            COMMAND
            "${JAVA_KEYTOOL}"
                -genkey -v
                -keystore "${DEBUG_KEYSTORE}"
                -alias androiddebugkey
                -storepass android
                -keypass android
                -keyalg RSA
                -validity 10950 # 30 years
                -dname "CN=Android Debug,O=Android,C=US"
            RESULT_VARIABLE KEYTOOL_RET
        )
        if(NOT KEYTOOL_RET EQUAL 0)
            message(FATAL_ERROR "Failed to create debug keystore"
                                " (keytool exit code: ${KEYTOOL_RET})")
        endif()
    endif()

    # For debug builds, we can allow not providing a signing config
    set(REPLACE_ME_KEYSTORE_PATH       "${DEBUG_KEYSTORE}")
    set(REPLACE_ME_KEYSTORE_PASSPHRASE "android")
    set(REPLACE_ME_KEY_ALIAS           "androiddebugkey")
    set(REPLACE_ME_KEY_PASSPHRASE      "android")

    set(MBP_SIGN_CONFIG_PATH ${CMAKE_BINARY_DIR}/SigningConfig.debug.prop
        CACHE FILEPATH "Signing configuration file path" FORCE)

    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/SigningConfig.prop.in
        ${MBP_SIGN_CONFIG_PATH}
        @ONLY
    )
endif()

# The user must provide a signing config

if(NOT MBP_SIGN_CONFIG_PATH)
    message(FATAL_ERROR
            "MBP_SIGN_CONFIG_PATH must be set to the path of the "
            "code signing configuration file. "
            "See cmake/SigningConfig.prop.in for more details")
endif()

read_signing_config(MBP_SIGN_JAVA_ ${MBP_SIGN_CONFIG_PATH})

# Try to prevent using debug key in release builds
if(${MBP_BUILD_TYPE} STREQUAL release
        AND "${MBP_SIGN_JAVA_KEY_ALIAS}" STREQUAL "androiddebugkey")
    message(FATAL_ERROR "Do not use a debug key in a release build")
endif()

# Export certificate to file
execute_process(
    COMMAND
    "${JAVA_KEYTOOL}"
    -exportcert
    -keystore "${MBP_SIGN_JAVA_KEYSTORE_PATH}"
    -storepass "${MBP_SIGN_JAVA_KEYSTORE_PASSPHRASE}"
    -alias "${MBP_SIGN_JAVA_KEY_ALIAS}"
    -file "${CMAKE_BINARY_DIR}/java_keystore_cert.des"
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to extract certificate from keystore")
endif()

set(PKCS12_KEYSTORE_PATH "${CMAKE_BINARY_DIR}/java_keystore.p12")

# Export keystore to encrypted PKCS12
execute_process(
    COMMAND
    "${JAVA_KEYTOOL}"
    -importkeystore
    -srcstoretype JKS
    -deststoretype PKCS12
    -srckeystore "${MBP_SIGN_JAVA_KEYSTORE_PATH}"
    -destkeystore "${PKCS12_KEYSTORE_PATH}"
    -srcstorepass "${MBP_SIGN_JAVA_KEYSTORE_PASSPHRASE}"
    -deststorepass "${MBP_SIGN_JAVA_KEYSTORE_PASSPHRASE}"
    -srcalias "${MBP_SIGN_JAVA_KEY_ALIAS}"
    -destalias "${MBP_SIGN_JAVA_KEY_ALIAS}"
    -srckeypass "${MBP_SIGN_JAVA_KEY_PASSPHRASE}"
    -destkeypass "${MBP_SIGN_JAVA_KEY_PASSPHRASE}"
    -noprompt
    RESULT_VARIABLE ret
)
if(NOT ret EQUAL 0)
    message(FATAL_ERROR "Failed to convert keystore to PKCS12")
endif()

# Read des certificate as hex
file(
    READ
    ${CMAKE_BINARY_DIR}/java_keystore_cert.des
    MBP_SIGN_JAVA_CERT_HEX
    HEX
)

function(add_sign_files_target name)
    set(files)
    foreach(file ${ARGN})
        string(CONCAT files "${files}" "${file}" "$<SEMICOLON>")
    endforeach()
    add_custom_target(
        ${name} ALL
        ${CMAKE_COMMAND}
            -DSIGN_FILES=${files}
            -P ${CMAKE_BINARY_DIR}/cmake/SignFiles.cmake
        COMMENT "File signing target '${name}'"
        VERBATIM
    )
    add_dependencies(${name} hosttools)
endfunction()
