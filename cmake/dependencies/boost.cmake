if(ANDROID)
    list(APPEND CMAKE_FIND_ROOT_PATH ${THIRD_PARTY_BOOST_DIR}/${ANDROID_ABI})
endif()

find_package(
    Boost
    REQUIRED
    COMPONENTS filesystem
)
