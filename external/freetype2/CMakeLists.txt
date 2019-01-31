project(freetype2 LANGUAGES C ASM)

add_library(
    freetype2-static
    STATIC
    upstream/src/autofit/autofit.c
    upstream/src/base/ftbase.c
    upstream/src/base/ftbbox.c
    upstream/src/base/ftbitmap.c
    upstream/src/base/ftfntfmt.c
    upstream/src/base/ftfstype.c
    upstream/src/base/ftgasp.c
    upstream/src/base/ftglyph.c
    upstream/src/base/ftinit.c
    upstream/src/base/ftlcdfil.c
    upstream/src/base/ftmm.c
    upstream/src/base/ftstroke.c
    upstream/src/base/fttype1.c
    upstream/src/base/ftsystem.c
    upstream/src/cid/type1cid.c
    upstream/src/cff/cff.c
    upstream/src/gzip/ftgzip.c
    upstream/src/psaux/psaux.c
    upstream/src/pshinter/pshinter.c
    upstream/src/psnames/psnames.c
    upstream/src/raster/raster.c
    upstream/src/sfnt/sfnt.c
    upstream/src/smooth/smooth.c
    upstream/src/truetype/truetype.c
    upstream/src/type1/type1.c
)

target_include_directories(
    freetype2-static
    PUBLIC
    upstream/include
)

target_compile_definitions(
    freetype2-static
    PRIVATE
    -DDARWIN_NO_CARBON
    -DFT2_BUILD_LIBRARY
)

target_compile_options(
    freetype2-static
    PRIVATE
    -O2
    -Wno-cast-align
    -Wno-cast-qual
    -Wno-comma
    -Wno-documentation
    -Wno-missing-variable-declarations
    -Wno-shorten-64-to-32
    -Wno-sign-conversion
    -Wno-unused-parameter
    -Wno-unused-variable
)

target_link_libraries(
    freetype2-static
    PUBLIC
    png-static
    ZLIB::ZLIB
)
