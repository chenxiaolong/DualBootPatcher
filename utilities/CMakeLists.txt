set(AROMA_VER "2.70RC2")

file(
    DOWNLOAD
    #"http://forum.xda-developers.com/devdb/project/dl/?id=286&task=get"
    "https://snapshots.noobdev.io/mirror/aroma-2.70RC2.zip"
    ${CMAKE_CURRENT_BINARY_DIR}/aroma-${AROMA_VER}.zip
    EXPECTED_HASH MD5=a77c4993803db28d53cd7e6a37ec73b5
    EXPECTED_HASH SHA512=44abff7bd536908ae8cde9a17e1fb334b59561e115f54b23bf910e1f7920b6f35ab078d3353db65c3526e25c0be27cd592470145063cafd4e05418e4bece775f
    TLS_VERIFY ON
    SHOW_PROGRESS
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/create.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/create.sh
    @ONLY
)
