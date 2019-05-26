# Copyright (C) 2017-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

FROM @REPO@:@VERSION@-@RELEASE@-base

# Install Android SDK
ARG ANDROID_SDK_VERSION=4333796
ARG ANDROID_SDK_CHECKSUM=92ffee5a1d98d856634e8b71132e8a95d96c83a63fde1099be3d86df3106def9
ENV ANDROID_HOME /opt/android-sdk

RUN mkdir ${ANDROID_HOME} \
    && cd ${ANDROID_HOME} \
    && dnf install -y aria2 unzip \
    && aria2c -x4 https://dl.google.com/android/repository/sdk-tools-linux-${ANDROID_SDK_VERSION}.zip \
        --check-integrity --checksum sha-256=${ANDROID_SDK_CHECKSUM} \
    && unzip -q sdk-tools-linux-${ANDROID_SDK_VERSION}.zip \
    && rm sdk-tools-linux-${ANDROID_SDK_VERSION}.zip \
    && dnf remove -y aria2 unzip \
    && dnf clean all

# Install Android SDK dependencies
RUN dnf -y install java-1.8.0-openjdk-devel glibc.i686 libstdc++.i686 which \
    && dnf clean all

# Set up path
ENV PATH ${PATH}:${ANDROID_NDK_HOME}:${ANDROID_HOME}/tools:${ANDROID_HOME}/tools/bin:${ANDROID_HOME}/platform-tools

# Install Android SDK components
RUN yes | sdkmanager \
    'build-tools;28.0.3' \
    'extras;android;m2repository' \
    'extras;google;m2repository' \
    'platforms;android-28' \
    'platform-tools' \
    'tools' \
    | grep -v '\[[= ]\+\]'
