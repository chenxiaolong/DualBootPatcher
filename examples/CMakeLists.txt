if(${MBP_BUILD_TARGET} STREQUAL desktop)
    # libmbpatcher tester

    add_executable(
        libmbpatcher_test
        libmbpatcher_test.cpp
    )
    target_link_libraries(
        libmbpatcher_test
        PRIVATE
        mbpatcher-shared
        mbpio-static
        mbdevice-shared
        mblog-shared
    )

    if(NOT MSVC)
        set_target_properties(
            libmbpatcher_test
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
        PRIVATE
        mbsparse-shared
        mblog-shared
        mbcommon-shared
    )

    if(NOT MSVC)
        set_target_properties(
            desparse
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()

    # binary grep tool

    add_executable(
        bingrep
        bingrep.cpp
    )
    target_link_libraries(
        bingrep
        PRIVATE
        mbcommon-shared
    )

    if(NOT MSVC)
        set_target_properties(
            bingrep
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()
endif()

if(${MBP_BUILD_TARGET} STREQUAL desktop OR ${MBP_BUILD_TARGET} STREQUAL android-app)
    # boot image compare tool

    add_executable(
        bootimg_compare
        bootimg_compare.cpp
    )
    target_link_libraries(
        bootimg_compare
        PRIVATE
        mbbootimg-shared
    )
    set_target_properties(bootimg_compare PROPERTIES POSITION_INDEPENDENT_CODE 1)

    if(NOT MSVC)
        set_target_properties(
            bootimg_compare
            PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED 1
        )
    endif()
endif()
