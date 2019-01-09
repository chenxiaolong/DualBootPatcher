if(${MBP_BUILD_TARGET} STREQUAL desktop)
    # libmbpatcher tester

    add_executable(
        libmbpatcher_test
        libmbpatcher_test.cpp
    )
    target_link_libraries(
        libmbpatcher_test
        PRIVATE
        interface.global.CXXVersion
        mbpatcher-shared
        mbpio-shared
        mbdevice-shared
        mblog-shared
    )

    # desparse tool

    add_executable(
        desparse
        desparse.cpp
    )
    target_link_libraries(
        desparse
        PRIVATE
        interface.global.CXXVersion
        mbsparse-shared
        mblog-shared
        mbcommon-shared
    )

    # binary grep tool

    add_executable(
        bingrep
        bingrep.cpp
    )
    target_link_libraries(
        bingrep
        PRIVATE
        interface.global.CXXVersion
        mbcommon-shared
    )
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
        interface.global.CXXVersion
        mbbootimg-shared
    )
endif()

if(UNIX AND (${MBP_BUILD_TARGET} STREQUAL desktop
        OR ${MBP_BUILD_TARGET} STREQUAL android-system))
    # Simple frontend to libmbsystrace

    add_executable(
        mbtrace
        mbtrace.cpp
    )
    target_link_libraries(
        mbtrace
        PRIVATE
        interface.global.CXXVersion
        mbsystrace-static
    )
endif()
