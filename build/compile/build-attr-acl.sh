#!/bin/bash

set -e

export ARM_TOOLCHAIN_PREFIX="arm-linux-gnueabihf"
export HOST_TOOLCHAIN_PREFIX="x86_64-unknown-linux-gnu"

if [ ! -d gcc-linaro-arm-linux-gnueabihf-4.8-2013.11_linux ]; then
  wget http://releases.linaro.org/13.11/components/toolchain/binaries/gcc-linaro-arm-linux-gnueabihf-4.8-2013.11_linux.tar.xz
  tar Jxvf gcc-linaro-arm-linux-gnueabihf-4.8-2013.11_linux.tar.xz
fi

export GCC_DIR="$(pwd)/gcc-linaro-arm-linux-gnueabihf-4.8-2013.11_linux/bin"

TEMPDIR=$(mktemp -d)
export PATH="${PATH}:${GCC_DIR}"

pushd "${TEMPDIR}"
wget 'http://download.savannah.gnu.org/releases/attr/attr-2.4.47.src.tar.gz'
tar zxvf attr-2.4.47.src.tar.gz

pushd attr-2.4.47/

./configure \
  --host=${ARM_TOOLCHAIN_PREFIX} \
  --build=${HOST_TOOLCHAIN_PREFIX} \
  --prefix=$(readlink -f ../target) \
  --enable-shared \
  --enable-static \
  --disable-lib64 \
  --disable-gettext

make

make install-dev
make install-lib

# libtool doesn't link the libraries statically
pushd getfattr
${ARM_TOOLCHAIN_PREFIX}-gcc \
  -static \
  -o ../../target/getfattr \
  getfattr.o \
  ../libmisc/.libs/libmisc.a \
  ../libattr/.libs/libattr.a
popd

pushd setfattr
${ARM_TOOLCHAIN_PREFIX}-gcc \
  -static \
  -o ../../target/setfattr \
  setfattr.o \
  ../libmisc/.libs/libmisc.a \
  ../libattr/.libs/libattr.a
popd

popd

wget 'http://download.savannah.gnu.org/releases/acl/acl-2.2.52.src.tar.gz'
tar zxvf acl-2.2.52.src.tar.gz

pushd acl-2.2.52

export CFLAGS="-I$(readlink -f ../target)/include"
export LDFLAGS="-L$(readlink -f ../target)/lib"

./configure \
  --host=${ARM_TOOLCHAIN_PREFIX} \
  --build=${HOST_TOOLCHAIN_PREFIX} \
  --prefix=$(readlink -f ../target) \
  --disable-shared \
  --enable-static \
  --disable-lib64 \
  --disable-gettext

make

pushd getfacl
${ARM_TOOLCHAIN_PREFIX}-gcc \
  ${CFLAGS} ${LDFLAGS} \
  -static \
  -o ../../target/getfacl \
  getfacl.o \
  user_group.o \
  ../libmisc/.libs/libmisc.a \
  ../libacl/.libs/libacl.a \
  -lattr
popd

pushd setfacl
${ARM_TOOLCHAIN_PREFIX}-gcc \
  ${CFLAGS} ${LDFLAGS} \
  -static \
  -o ../../target/setfacl \
  setfacl.o \
  do_set.o \
  sequence.o \
  parse.o \
  ../libmisc/.libs/libmisc.a \
  ../libacl/.libs/libacl.a \
  -lattr
popd

popd

pushd target
find -maxdepth 1 -mindepth 1 -type d | xargs rm -rf
${ARM_TOOLCHAIN_PREFIX}-strip getfattr setfattr getfacl setfacl
if which upx >/dev/null; then
  upx -9 getfattr setfattr getfacl setfacl
fi
popd

popd

cp "${TEMPDIR}"/target/{getfattr,setfattr,getfacl,setfacl} .

rm -rf "${TEMPDIR}"
