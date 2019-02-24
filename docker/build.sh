#!/bin/bash

set -eu

cd "$(dirname "${BASH_SOURCE[0]}")"
source common.sh

image=${repo}:${version}-${release}

docker build \
    --force-rm \
    -t "${image}" \
    .

echo "${image}" > image.txt
