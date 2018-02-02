if(ANDROID)
    message(STATUS "NOTE: Android actually uses BoringSSL")
endif()

find_package(OpenSSL REQUIRED)
