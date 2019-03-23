# We don't need the keys for building the host tools
if(${MBP_BUILD_TARGET} STREQUAL hosttools)
    return()
endif()

# Signing config
set(MBP_SIGN_CONFIG_PATH "" CACHE FILEPATH "Signing configuration file path")

# The user must provide a signing config
if(NOT MBP_SIGN_CONFIG_PATH)
    message(FATAL_ERROR
            "MBP_SIGN_CONFIG_PATH must be set to the path of the "
            "code signing configuration file. "
            "See cmake/SigningConfig.prop.in for more details")
endif()

read_signing_config(MBP_SIGN_ ${MBP_SIGN_CONFIG_PATH})

# Read public key as hex
file(
    READ
    ${MBP_SIGN_PUBLIC_KEY_PATH}
    MBP_SIGN_PUBLIC_KEY
)
