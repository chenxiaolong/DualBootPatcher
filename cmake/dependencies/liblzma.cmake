if(ANDROID)
    set(MBP_LIBLZMA_INCLUDES
        ${THIRD_PARTY_LIBLZMA_DIR}/${ANDROID_ABI}/include)
    set(MBP_LIBLZMA_LIBRARIES
        ${THIRD_PARTY_LIBLZMA_DIR}/${ANDROID_ABI}/lib/liblzma.a)
else()
    find_package(LibLZMA)
    if(NOT LIBLZMA_FOUND)
        message(FATAL_ERROR "LibLZMA is not found!")
    endif()

    set(MBP_LIBLZMA_INCLUDES ${LIBLZMA_INCLUDE_DIRS})
    set(MBP_LIBLZMA_LIBRARIES ${LIBLZMA_LIBRARIES})
endif()
