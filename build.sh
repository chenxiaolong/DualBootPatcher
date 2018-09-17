#!/bin/bash

export ARCH=arm
export CROSS_COMPILE=/home/adam/arm-eabi-4.8/bin/arm-eabi-
export LOCALVERSION=-UlTRA
export KBUILD_BUILD_USER="AdaM"
export KBUILD_BUILD_HOST="Gaza"
mkdir output

make -C $(pwd) O=output VARIANT_DEFCONFIG=hq8321_tb_b2b_l_defconfig
make -C $(pwd) O=output

cp output/arch/arm/boot/Image $(pwd)/arch/arm/boot/zImage
