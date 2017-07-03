#!/bin/bash
echo "Fix Previous Buid"
sudo dpkg --configure -a && sudo apt install -f && sudo apt-get autoremove &> /dev/null
echo "Installing Packages"
sudo apt-get install -y openjdk-8-jdk libjansson-dev openssl libssl-dev libyaml-cpp-dev libboost-dev ccache python-minimal &> /dev/null
echo "Addidtion Packages"

echo "Downloading cmake"
mkdir ~/dev && cd ~/dev && wget https://cmake.org/files/v3.6/cmake-3.6.3.tar.gz && tar xzf cmake-3.6.3.tar.gz && cd ~/dev/cmake-3.6.3
echo "Building cmake"
sudo sed -i 's|cmake_options="-DCMAKE_BOOTSTRAP=1"|cmake_options="-DCMAKE_BOOTSTRAP=1 -DCMAKE_USE_OPENSSL=ON"|' ~/dev/cmake-3.6.3/bootstrap && sudo ./bootstrap && sudo make && sudo make install &> come here

echo "get Android NDK R15 & SDK R25.0.3"
cd ~/dev && mkdir android-ndk && cd ~/dev/android-ndk
echo "Downloading NDK"
wget https://dl.google.com/android/repository/android-ndk-r15-linux-x86_64.zip &> come here && unzip -qq android-ndk-r15-linux-x86_64.zip
echo "Downloading SDK Build Tools"
cd ~/dev && mkdir android-sdk && cd ~/dev/android-sdk && mkdir build-tools && cd ~/dev/android-sdk/build-tools
wget https://dl.google.com/android/repository/tools_r25.0.3-linux.zip &> come here && unzip -qq tools_r25.0.3-linux.zip -d ~/dev/android-sdk/build-tools/25.0.3
echo "Downloading SDK Platform"
cd ~/dev/android-sdk && mkdir platforms && cd ~/dev/android-sdk/platforms
wget https://dl.google.com/android/repository/platform-25_r03.zip && unzip -qq platform-25_r03.zip -d ~/dev/android-sdk/platforms/android-25
echo "Set Variables"
env | grep CCACHE
env | grep NDK
echo "export USE_CCACHE=1" >> ~/.bashrc
echo "export CCACHE_DIR=~/out/ccache" >> ~/.bashrc
echo "export ANDROID_HOME=~/dev/android-sdk" >> ~/.bashrc
echo "export ANDROID_NDK_HOME=~/dev/android-ndk/android-ndk-r15" >> ~/.bashrc
echo "export ANDROID_NDK=~/dev/android-ndk/android-ndk-r15" >> ~/.bashrc
echo "reload bash environment"
source ~/.bashrc
env | grep CCACHE
env | grep NDK
echo "set ccache size to 40 Gb"
mkdir ~/out
mkdir ~/out/ccache
ccache -M 40G
echo "clone DualBootPatcher source"
cd ~/dev
git clone --recursive https://github.com/yshalsager/DualbootPatcher.git DualBootPatcher && cd DualBootPatcher && mkdir build && cd build
echo "Building..."
cmake .. \
   -DMBP_BUILD_TARGET=android \
   -DMBP_BUILD_TYPE=debug
echo "Finishing Build"
make clean && make && rm -rf assets && cpack -G TXZ && make apk && make android-system_armeabi-v7a && make -C data/devices && ./utilities/create.sh
