#!/bin/bash

cd "$(dirname "${0}")"

url="http://forum.xda-developers.com/devdb/project/dl/?id=286&task=get"
target=aromawrapper.zip

rm -f ${target}

pushd aromawrapper
zip -r ../${target} META-INF/
popd

if [[ ! -f aroma.zip ]]; then
  wget -O aroma.zip "${url}"
fi

unzip aroma.zip META-INF/com/google/android/update-binary
zip ${target} META-INF/com/google/android/update-binary

rm -r META-INF/
