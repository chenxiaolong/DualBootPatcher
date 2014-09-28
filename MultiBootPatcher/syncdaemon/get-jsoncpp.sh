#!/bin/bash

set -e

cd "$(dirname "${0}")"

if [ ! -d jsoncpp ]; then
    git clone https://github.com/open-source-parsers/jsoncpp.git
fi

pushd jsoncpp/
    git fetch
    git checkout 033677cc1a063cfbc9b939cadd4946a7c8e51f89
    python2 amalgamate.py
popd

rm -rf jsoncpp-dist/
mkdir jsoncpp-dist/
mv jsoncpp/dist/* jsoncpp-dist/
