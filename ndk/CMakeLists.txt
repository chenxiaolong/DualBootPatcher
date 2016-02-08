set(MBP_GLOBAL_INCLUDE_DIRS_STR "")
foreach(_dir ${MBP_GLOBAL_INCLUDE_DIRS})
    set(MBP_GLOBAL_INCLUDE_DIRS_STR "${MBP_GLOBAL_INCLUDE_DIRS_STR} ${CMAKE_SOURCE_DIR}/${_dir}")
endforeach()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/common.mk.in
    ${CMAKE_CURRENT_BINARY_DIR}/common.mk
    @ONLY
)
