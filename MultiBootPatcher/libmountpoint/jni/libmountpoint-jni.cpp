/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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
#include "libmountpoint.h"

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jobjectArray JNICALL Java_com_github_chenxiaolong_dualbootpatcher_freespace_FreeSpaceFragment_getMountPoints(
        JNIEnv *env, jclass clazz) {
    std::vector<std::string> mountpoints = get_mount_points();

    jobjectArray ret = (jobjectArray) env->NewObjectArray(mountpoints.size(),
            env->FindClass("java/lang/String"), NULL);

    for (unsigned int i = 0; i < mountpoints.size(); i++) {
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(mountpoints[i].c_str()));
    }

    return ret;
}

JNIEXPORT jstring JNICALL Java_com_github_chenxiaolong_dualbootpatcher_freespace_FreeSpaceFragment_getMntFsname(
        JNIEnv *env, jclass clazz, jstring jMountpoint) {
    const char *mountpoint = env->GetStringUTFChars(jMountpoint, JNI_FALSE);

    std::string ret = get_mnt_fsname(mountpoint);
    jstring jret = env->NewStringUTF(ret.c_str());

    env->ReleaseStringUTFChars(jMountpoint, mountpoint);

    return jret;
}

JNIEXPORT jstring JNICALL Java_com_github_chenxiaolong_dualbootpatcher_freespace_FreeSpaceFragment_getMntType(
        JNIEnv *env, jclass clazz, jstring jMountpoint) {
    const char *mountpoint = env->GetStringUTFChars(jMountpoint, JNI_FALSE);

    std::string ret = get_mnt_type(mountpoint);
    jstring jret = env->NewStringUTF(ret.c_str());

    env->ReleaseStringUTFChars(jMountpoint, mountpoint);

    return jret;
}

JNIEXPORT jstring JNICALL Java_com_github_chenxiaolong_dualbootpatcher_freespace_FreeSpaceFragment_getMntOpts(
        JNIEnv *env, jclass clazz, jstring jMountpoint) {
    const char *mountpoint = env->GetStringUTFChars(jMountpoint, JNI_FALSE);

    std::string ret = get_mnt_opts(mountpoint);
    jstring jret = env->NewStringUTF(ret.c_str());

    env->ReleaseStringUTFChars(jMountpoint, mountpoint);

    return jret;
}

JNIEXPORT jint JNICALL Java_com_github_chenxiaolong_dualbootpatcher_freespace_FreeSpaceFragment_getMntFreq(
        JNIEnv *env, jclass clazz, jstring jMountpoint) {
    const char *mountpoint = env->GetStringUTFChars(jMountpoint, JNI_FALSE);

    jint ret = get_mnt_freq(mountpoint);

    env->ReleaseStringUTFChars(jMountpoint, mountpoint);

    return ret;
}

JNIEXPORT jint JNICALL Java_com_github_chenxiaolong_dualbootpatcher_freespace_FreeSpaceFragment_getMntPassno(
        JNIEnv *env, jclass clazz, jstring jMountpoint) {
    const char *mountpoint = env->GetStringUTFChars(jMountpoint, JNI_FALSE);

    jint ret = get_mnt_passno(mountpoint);

    env->ReleaseStringUTFChars(jMountpoint, mountpoint);

    return ret;
}

JNIEXPORT jlong JNICALL Java_com_github_chenxiaolong_dualbootpatcher_freespace_FreeSpaceFragment_getMntTotalSize(
        JNIEnv *env, jclass clazz, jstring jMountpoint) {
    const char *mountpoint = env->GetStringUTFChars(jMountpoint, JNI_FALSE);

    jlong ret = get_mnt_total_size(mountpoint);

    env->ReleaseStringUTFChars(jMountpoint, mountpoint);

    return ret;
}

JNIEXPORT jlong JNICALL Java_com_github_chenxiaolong_dualbootpatcher_freespace_FreeSpaceFragment_getMntAvailSpace(
        JNIEnv *env, jclass clazz, jstring jMountpoint) {
    const char *mountpoint = env->GetStringUTFChars(jMountpoint, JNI_FALSE);

    jlong ret = get_mnt_avail_space(mountpoint);

    env->ReleaseStringUTFChars(jMountpoint, mountpoint);

    return ret;
}

#ifdef __cplusplus
}
#endif
