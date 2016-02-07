if(${MBP_BUILD_TARGET} STREQUAL desktop)
    # Allow libmbp headers to be found
    include_directories(${CMAKE_SOURCE_DIR})
    include_directories(${CMAKE_SOURCE_DIR}/libmbcommon/include)
    include_directories(${CMAKE_SOURCE_DIR}/libmblog/include)
    include_directories(${CMAKE_SOURCE_DIR}/libmbp/include)
    include_directories(${CMAKE_SOURCE_DIR}/libmbpio/include)
    include_directories(${CMAKE_SOURCE_DIR}/libmbsparse/include)

    # libmbp tester

    add_executable(
        libmbp_test
        libmbp_test.cpp
    )
    target_link_libraries(
        libmbp_test
        mbp
        mbpio
    )

    if(NOT MSVC)
        set_target_properties(
            libmbp_test
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # desparse tool

    add_executable(
        desparse
        desparse.cpp
    )
    target_link_libraries(
        desparse
        mbpio
        mbsparse
        mblog
    )

    if(NOT MSVC)
        set_target_properties(
            desparse
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()
endif()
