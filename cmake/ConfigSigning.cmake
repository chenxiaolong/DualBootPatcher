#include(CMakeDependentOption)
#include(GNUInstallDirs)

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
        set(DEBUG_KEYSTORE "$ENV{USERPROFILE}\\.android\\debug.keystore")
    else()
        set(DEBUG_KEYSTORE "$ENV{HOME}/.android/debug.keystore")
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
            RESULT_VARIABLE KEYTOOL_RET
        )
        if(NOT KEYTOOL EQUAL 0)
            message(FATAL_ERROR "Failed to create debug keystore"
                                " (keytool exit code: ${KEYTOOL RET})")
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
    CACHE STRING "Java signing keystore path")
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