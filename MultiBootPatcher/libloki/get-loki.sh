#!/bin/bash

set -e

cd "$(dirname "${0}")"

if [ ! -d loki ]; then
    git clone https://github.com/djrbliss/loki.git
fi

pushd loki/
git fetch
git checkout 14c37b73ad6401c952a39183f246a2d045aba9ae
git am ../0001-Add-loki-JNI-shared-library.patch
popd

rm -rf jni/
mkdir jni/
cp loki/*.{c,h,mk} loki/LICENSE.txt jni/
