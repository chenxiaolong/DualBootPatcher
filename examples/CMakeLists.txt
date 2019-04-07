if(${MBP_BUILD_TARGET} STREQUAL system)
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
        CLI11::CLI11
    )

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
        CLI11::CLI11
    )
endif()

if(UNIX AND (${MBP_BUILD_TARGET} STREQUAL system
        OR ${MBP_BUILD_TARGET} STREQUAL device))
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
