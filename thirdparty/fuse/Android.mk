# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfuse
LOCAL_SRC_FILES := \
	fuse/android/statvfs.c \
	fuse/lib/buffer.c \
	fuse/lib/cuse_lowlevel.c \
	fuse/lib/fuse.c \
	fuse/lib/fuse_kern_chan.c \
	fuse/lib/fuse_loop.c \
	fuse/lib/fuse_loop_mt.c \
	fuse/lib/fuse_lowlevel.c \
	fuse/lib/fuse_mt.c \
	fuse/lib/fuse_opt.c \
	fuse/lib/fuse_session.c \
	fuse/lib/fuse_signals.c \
	fuse/lib/helper.c \
	fuse/lib/mount.c \
	fuse/lib/mount_util.c \
	fuse/lib/ulockmgr.c
LOCAL_C_INCLUDES := \
	fuse/android \
	fuse/include
LOCAL_CFLAGS := \
	-D_FILE_OFFSET_BITS=64 \
	-DFUSE_USE_VERSION=26 \
	-fno-strict-aliasing
include $(BUILD_STATIC_LIBRARY)
