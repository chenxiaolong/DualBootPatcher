# We don't need the keys for building signtool
if(${MBP_BUILD_TARGET} STREQUAL signtool)
    return()
endif()

# Find Java's keytool
find_program(JAVA_KEYTOOL NAMES keytool)
if(NOT JAVA_KEYTOOL)
    message(FATAL_ERROR "The java 'keytool' program was not found")
endif()

# Create debug keystore if it doesn't exist
if(${MBP_BUILD_TYPE} STREQUAL debug)
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

    set(DEFAULT_SIGN_JAVA_KEYSTORE_PATH     "${DEBUG_KEYSTORE}")
    set(DEFAULT_SIGN_JAVA_KEYSTORE_PASSWORD "android")
    set(DEFAULT_SIGN_JAVA_KEY_ALIAS         "androiddebugkey")
    set(DEFAULT_SIGN_JAVA_KEY_PASSWORD      "android")
else()
    # No defaults, since the user should provide these
    set(DEFAULT_SIGN_JAVA_KEYSTORE_PATH     "")
    set(DEFAULT_SIGN_JAVA_KEYSTORE_PASSWORD "")
    set(DEFAULT_SIGN_JAVA_KEY_ALIAS         "")
    set(DEFAULT_SIGN_JAVA_KEY_PASSWORD      "")
endif()

# User's signing options
set(MBP_SIGN_JAVA_KEYSTORE_PATH     "${DEFAULT_SIGN_JAVA_KEYSTORE_PATH}"
    CACHE FILEPATH "Java signing keystore path")
set(MBP_SIGN_JAVA_KEYSTORE_PASSWORD "${DEFAULT_SIGN_JAVA_KEYSTORE_PASSWORD}"
    CACHE STRING "Java signing keystore password")
set(MBP_SIGN_JAVA_KEY_ALIAS         "${DEFAULT_SIGN_JAVA_KEY_ALIAS}"
    CACHE STRING "Java signing key alias")
set(MBP_SIGN_JAVA_KEY_PASSWORD      "${DEFAULT_SIGN_JAVA_KEY_PASSWORD}"
    CACHE STRING "Java signing key password")

# Ensure all signing variables are defined
set(SIGNING_UNDEFINED_VARS "")
if(NOT MBP_SIGN_JAVA_KEYSTORE_PATH)
    list(APPEND SIGNING_UNDEFINED_VARS MBP_SIGN_JAVA_KEYSTORE_PATH)
endif()
if(NOT MBP_SIGN_JAVA_KEYSTORE_PASSWORD)
    list(APPEND SIGNING_UNDEFINED_VARS MBP_SIGN_JAVA_KEYSTORE_PASSWORD)
endif()
if(NOT MBP_SIGN_JAVA_KEY_ALIAS)
    list(APPEND SIGNING_UNDEFINED_VARS MBP_SIGN_JAVA_KEY_ALIAS)
endif()
if(NOT MBP_SIGN_JAVA_KEY_PASSWORD)
    list(APPEND SIGNING_UNDEFINED_VARS MBP_SIGN_JAVA_KEY_PASSWORD)
endif()

if(SIGNING_UNDEFINED_VARS)
    set(ERROR_MSG "The following code signing variables must be specified:")
    foreach(SIGNING_UNDEFINED_VAR ${SIGNING_UNDEFINED_VARS})
        set(ERROR_MSG "${ERROR_MSG}\n- ${SIGNING_UNDEFINED_VAR}")
    endforeach()
    message(FATAL_ERROR "${ERROR_MSG}")
endif()

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
    -storepass "${MBP_SIGN_JAVA_KEYSTORE_PASSWORD}"
    -alias "${MBP_SIGN_JAVA_KEY_ALIAS}"
    -file "${CMAKE_BINARY_DIR}/java_keystore_cert.des"
)

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
    -srcstorepass "${MBP_SIGN_JAVA_KEYSTORE_PASSWORD}"
    -deststorepass "${MBP_SIGN_JAVA_KEYSTORE_PASSWORD}"
    -srcalias "${MBP_SIGN_JAVA_KEY_ALIAS}"
    -destalias "${MBP_SIGN_JAVA_KEY_ALIAS}"
    -srckeypass "${MBP_SIGN_JAVA_KEY_PASSWORD}"
    -destkeypass "${MBP_SIGN_JAVA_KEY_PASSWORD}"
    -noprompt
)

# Read des certificate as hex
file(
    READ
    ${CMAKE_BINARY_DIR}/java_keystore_cert.des
    MBP_SIGN_JAVA_CERT_HEX
    HEX
)
