#!/bin/bash

set -e

icon_file=icon.png
icon_sizes=(256 64 48 40 32 24 20 16)
icon_files=()

for size in "${icon_sizes[@]}"; do
    convert -resize ${size}x${size} ${icon_file} output-${size}.ico
    icon_files+=(output-${size}.ico)
done

convert "${icon_files[@]}" icon.ico
rm "${icon_files[@]}"
