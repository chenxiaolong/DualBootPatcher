set(MBP_ENABLE_TESTS TRUE CACHE BOOL "Enable building of tests")

if(MBP_ENABLE_TESTS)
    enable_testing()

    function(add_gtest_test target)
        set(output_file "${CMAKE_CURRENT_BINARY_DIR}/${target}.gtest.xml")

        add_test(
            NAME "${target}"
            COMMAND "${target}" "--gtest_output=xml:${output_file}"
        )
    endfunction()
endif()
