# Find the LZO include directory and library
#
# LZO_INCLUDE_DIR - Where to find <lzo/lzo1x.h>
# LZO_LIBRARIES   - List of lzo libraries
# LZO_FOUND       - True if lzo found

# Find include directory
find_path(LZO_INCLUDE_DIR NAMES lzo/lzo1x.h)

# Find library
find_library(LZO_LIBRARY NAMES lzo2 liblzo2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LZO DEFAULT_MSG LZO_INCLUDE_DIR LZO_LIBRARY
)

if(LZO_FOUND)
    set(LZO_LIBRARIES ${LZO_LIBRARY})
endif()

mark_as_advanced(LZO_INCLUDE_DIR LZO_LIBRARY)
