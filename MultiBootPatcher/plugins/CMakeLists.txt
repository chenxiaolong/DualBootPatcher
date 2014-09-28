set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

# Allow libdbp headers to be found
include_directories(${CMAKE_SOURCE_DIR})

# Allow plugins to link to other plugins
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(libdbp-autopatcher-chenxiaolong)
add_subdirectory(libdbp-autopatcher-jflte)
add_subdirectory(libdbp-autopatcher-patchfile)
add_subdirectory(libdbp-autopatcher-standard)
add_subdirectory(libdbp-patcher-multiboot)
add_subdirectory(libdbp-patcher-primaryupgrade)
add_subdirectory(libdbp-patcher-syncdaemonupdate)
add_subdirectory(libdbp-ramdisk-bacon)
add_subdirectory(libdbp-ramdisk-common)
add_subdirectory(libdbp-ramdisk-d800)
add_subdirectory(libdbp-ramdisk-falcon)
add_subdirectory(libdbp-ramdisk-hammerhead)
add_subdirectory(libdbp-ramdisk-jflte)
add_subdirectory(libdbp-ramdisk-qcom)