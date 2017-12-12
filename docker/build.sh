#!/bin/bash

set -eu

variants=(base android mingw linux)

cd "$(dirname "${BASH_SOURCE[0]}")"
source common.sh

if [[ ! -d generated ]]; then
    echo >&2 "Run generate.sh first"
    exit 1
fi

for variant in "${variants[@]}"; do
    docker build \
        --force-rm \
        -f "generated/Dockerfile.${variant}" \
        -t "${repo}:${version}-${release}-${variant}" \
        .
done
