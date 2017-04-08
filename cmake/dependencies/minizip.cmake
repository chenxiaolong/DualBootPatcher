# Always use bundled minizip
set(USE_AES OFF CACHE BOOL "enables building of aes library" FORCE)
add_subdirectory(external/minizip)
