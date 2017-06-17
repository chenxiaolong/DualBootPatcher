#!/bin/bash

set -eu

cd "$(dirname "${BASH_SOURCE[0]}")"
source common.sh

mkdir -p generated
echo -n > generated/images.properties

for template in template/Dockerfile.*.in; do
    filename=${template##*/}
    filename=${filename%.in}
    target=generated/${filename}
    variant=${filename#Dockerfile.}

    sed \
        -e "s,@REPO@,${repo},g" \
        -e "s,@VERSION@,${version},g" \
        -e "s,@RELEASE@,${release},g" \
        < "${template}" \
        > "${target}"

    echo "${variant}=${repo}:${version}-${release}-${variant}" \
        >> generated/images.properties
done
