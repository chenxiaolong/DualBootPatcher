find_path(MBP_JSONCPP_INCLUDES json/json.h)
find_library(MBP_JSONCPP_LIBRARIES NAMES jsoncpp libjsoncpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    JSONCPP DEFAULT_MSG MBP_JSONCPP_LIBRARIES MBP_JSONCPP_INCLUDES
)
if(NOT JSONCPP_FOUND)
    message(FATAL_ERROR "jsoncpp library was not found")
endif()
