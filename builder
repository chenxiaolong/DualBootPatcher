#!/bin/bash
sudo apt-get install openjdk-8-jdk libjansson-dev openssl libssl-dev libyaml-cpp-dev libboost-dev ccache python-minimal

mkdir ~/dev && cd ~/dev && wget https://cmake.org/files/v3.6/cmake-3.6.3.tar.gz && tar xzf cmake-3.6.3.tar.gz && cd ~/dev/cmake-3.6.3
sudo sed -i 's|cmake_options="-DCMAKE_BOOTSTRAP=1"|cmake_options="-DCMAKE_BOOTSTRAP=1 -DCMAKE_USE_OPENSSL=ON"|' ~/dev/cmake-3.6.3/bootstrap && sudo ./bootstrap && sudo make && sudo make install &> come here

# get Android NDK R15 & SDK R25.0.3
cd ~/dev && mkdir android-ndk
cd ~/dev/android-ndk
# git clone https://android.googlesource.com/platform/ndk ~/dev/android-ndk
wget https://dl.google.com/android/repository/android-ndk-r15-linux-x86_64.zip &> come here && unzip -qq android-ndk-r15-linux-x86_64.zip
cd ~/dev && mkdir android-sdk && cd ~/dev/android-sdk && mkdir build-tools && cd ~/dev/android-sdk/build-tools
wget https://dl.google.com/android/repository/tools_r25.0.3-linux.zip &> come here
unzip -qq tools_r25.0.3-linux.zip -d ~/dev/android-sdk/build-tools/25.0.3
cd ~/dev/android-sdk && mkdir platforms && cd ~/dev/android-sdk/platforms
wget https://dl.google.com/android/repository/platform-25_r03.zip 
unzip -qq platform-25_r03.zip -d ~/dev/android-sdk/platforms/android-25

env | grep CCACHE
env | grep NDK
echo "export USE_CCACHE=1" >> ~/.bashrc
echo "export CCACHE_DIR=~/out/ccache" >> ~/.bashrc
echo 'export ANDROID_HOME=~/dev/android-sdk' >> ~/.bashrc
echo 'export ANDROID_NDK_HOME=~/dev/android-ndk/android-ndk-r15' >> ~/.bashrc
echo 'export ANDROID_NDK=~/dev/android-ndk/android-ndk-r15' >> ~/.bashrc
# reload bash environment
source ~/.bashrc
env | grep CCACHE
env | grep NDK
# set ccache size to 40 Gb
mkdir ~/out
mkdir ~/out/ccache
ccache -M 40G

# clone DualBootPatcher source here
cd ~/dev
git clone --recursive https://github.com/yshalsager/DualbootPatcher.git && sleep 15s
cd ~/dev/DualBootPatcher && mkdir build && cd ~/dev/DualBootPatcher/build
# Build it now
wget https://github.com/yshalsager/DualBootPatcher/raw/master/runcmake
chmod 775 ~/dev/DualBootPatcher/build/runcmake
sh ~/dev/DualBootPatcher/build/runcmake
make clean && make && rm -rf assets && cpack -G TXZ && make apk && make android-system_armeabi-v7a && make -C data/devices && ./utilities/create.sh
