/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <jni.h>
#include "loki.h"

JNIEXPORT jint JNICALL Java_com_github_chenxiaolong_dualbootpatcher_settings_AppSharingUtils_lokiPatch(
        JNIEnv *env, jclass clazz, jstring jPartitionLabel, jstring jAbootImage,
        jstring jInImage, jstring jOutImage) {
    const char *partitionLabel = (*env)->GetStringUTFChars(env, jPartitionLabel, 0);
    const char *abootImage = (*env)->GetStringUTFChars(env, jAbootImage, 0);
    const char *inImage = (*env)->GetStringUTFChars(env, jInImage, 0);
    const char *outImage = (*env)->GetStringUTFChars(env, jOutImage, 0);

    jint ret = loki_patch(partitionLabel, abootImage, inImage, outImage);

    (*env)->ReleaseStringUTFChars(env, jPartitionLabel, partitionLabel);
    (*env)->ReleaseStringUTFChars(env, jAbootImage, abootImage);
    (*env)->ReleaseStringUTFChars(env, jInImage, inImage);
    (*env)->ReleaseStringUTFChars(env, jOutImage, outImage);

    return ret;
}

JNIEXPORT jint JNICALL Java_com_github_chenxiaolong_dualbootpatcher_settings_AppSharingUtils_lokiFlash(
        JNIEnv *env, jclass clazz, jstring jPartitionLabel, jstring jLokiImage) {
    const char *partitionLabel = (*env)->GetStringUTFChars(env, jPartitionLabel, 0);
    const char *lokiImage = (*env)->GetStringUTFChars(env, jLokiImage, 0);

    jint ret = loki_flash(partitionLabel, lokiImage);

    (*env)->ReleaseStringUTFChars(env, jPartitionLabel, partitionLabel);
    (*env)->ReleaseStringUTFChars(env, jLokiImage, lokiImage);

    return ret;
}

JNIEXPORT jint JNICALL Java_com_github_chenxiaolong_dualbootpatcher_settings_AppSharingUtils_lokiFind(
        JNIEnv *env, jclass clazz, jstring jAbootImage) {
    const char *abootImage = (*env)->GetStringUTFChars(env, jAbootImage, 0);

    jint ret = loki_find(abootImage);

    (*env)->ReleaseStringUTFChars(env, jAbootImage, abootImage);

    return ret;
}

JNIEXPORT jint JNICALL Java_com_github_chenxiaolong_dualbootpatcher_settings_AppSharingUtils_lokiUnlok(
        JNIEnv *env, jclass clazz, jstring jInImage, jstring jOutImage) {
    const char *inImage = (*env)->GetStringUTFChars(env, jInImage, 0);
    const char *outImage = (*env)->GetStringUTFChars(env, jOutImage, 0);

    jint ret = loki_unlok(inImage, outImage);

    (*env)->ReleaseStringUTFChars(env, jInImage, inImage);
    (*env)->ReleaseStringUTFChars(env, jOutImage, outImage);

    return ret;
}
