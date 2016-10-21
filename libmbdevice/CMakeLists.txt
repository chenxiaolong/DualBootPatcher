include_directories(${MBP_JANSSON_INCLUDES})

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

set(MBDEVICE_SOURCES
    src/device.c
    src/json.c
    src/validate.c
    src/internal/array.c
    src/internal/structs.c
)

if(${MBP_BUILD_TARGET} STREQUAL android-system)
    # Build static library

    add_library(mbdevice-static STATIC ${MBDEVICE_SOURCES})

    target_compile_definitions(
        mbdevice-static
        PRIVATE
        -DMB_LIBRARY
    )

    if(NOT MSVC)
        set_target_properties(
            mbdevice-static
            PROPERTIES
            C_STANDARD 99
            C_STANDARD_REQUIRED 1
        )
    endif()

    if(MBP_ENABLE_TESTS)
        add_executable(mbdevice-static_test_device tests/test_device.cpp)
        add_executable(mbdevice-static_test_json tests/test_json.cpp)
        target_link_libraries(
            mbdevice-static_test_device
            mbdevice-static
            ${GTEST_BOTH_LIBRARIES}
        )
        target_link_libraries(
            mbdevice-static_test_json
            mbdevice-static
            ${GTEST_BOTH_LIBRARIES}
        )

        if(NOT MSVC)
            set_target_properties(
                mbdevice-static_test_device
                mbdevice-static_test_json
                PROPERTIES
                CXX_STANDARD 11
                CXX_STANDARD_REQUIRED 1
            )
        endif()

        add_test(
            NAME mbdevice-static_test_device
            COMMAND mbdevice-static_test_device
        )
        add_test(
            NAME mbdevice-static_test_json
            COMMAND mbdevice-static_test_json
        )
    endif()
elseif(${MBP_BUILD_TARGET} STREQUAL android-app
        OR ${MBP_BUILD_TARGET} STREQUAL desktop
        OR ${MBP_BUILD_TARGET} STREQUAL hosttools)
    # Build shared library

    add_library(mbdevice-shared SHARED ${MBDEVICE_SOURCES})

    target_compile_definitions(
        mbdevice-shared
        PRIVATE
        -DMB_DYNAMIC_LINK
        -DMB_LIBRARY
    )

    set_target_properties(
        mbdevice-shared
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        OUTPUT_NAME mbdevice
    )

    if(NOT MSVC)
        set_target_properties(
            mbdevice-shared
            PROPERTIES
            C_STANDARD 99
            C_STANDARD_REQUIRED 1
        )
    endif()

    target_link_libraries(
        mbdevice-shared
        mbcommon-shared
        ${MBP_JANSSON_LIBRARIES}
    )

    # Install library
    install(
        TARGETS mbdevice-shared
        LIBRARY DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        RUNTIME DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
        #ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Libraries
    )

    if(MBP_ENABLE_TESTS)
        add_executable(mbdevice-shared_test_device tests/test_device.cpp)
        add_executable(mbdevice-shared_test_json tests/test_json.cpp)
        target_link_libraries(
            mbdevice-shared_test_device
            mbdevice-shared
            ${GTEST_BOTH_LIBRARIES}
        )
        target_link_libraries(
            mbdevice-shared_test_json
            mbdevice-shared
            ${GTEST_BOTH_LIBRARIES}
        )

        if(NOT MSVC)
            set_target_properties(
                mbdevice-shared_test_device
                mbdevice-shared_test_json
                PROPERTIES
                CXX_STANDARD 11
                CXX_STANDARD_REQUIRED 1
            )
        endif()

        add_test(
            NAME mbdevice-shared_test_device
            COMMAND mbdevice-shared_test_device
        )
        add_test(
            NAME mbdevice-shared_test_json
            COMMAND mbdevice-shared_test_json
        )
    endif()
endif()
