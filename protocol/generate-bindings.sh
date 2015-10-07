#!/bin/bash

cd "$(dirname "${0}")"

files=(
    v2/get_version.fbs
    v2/get_roms_list.fbs
    v2/get_builtin_rom_ids.fbs
    v2/get_current_rom.fbs
    v2/switch_rom.fbs
    v2/set_kernel.fbs
    v2/reboot.fbs
    v2/open.fbs
    v2/copy.fbs
    v2/chmod.fbs
    v2/loki_patch.fbs
    v2/wipe_rom.fbs
    v2/selinux_get_label.fbs
    v2/selinux_set_label.fbs
    request.fbs
    response.fbs
)

mkdir -p ../mbtool/protocol

flatc -c -o ../mbtool/protocol "${files[@]}"
flatc -j -o ../Android_GUI/src "${files[@]}"
