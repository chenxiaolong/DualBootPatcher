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

###

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

###

common_cflags := \
	-D_FILE_OFFSET_BITS=64 \
	-DPACKAGE=\"exfat\" \
	-DVERSION=\"$(VERSION)\"

include $(CLEAR_VARS)
LOCAL_MODULE := libexfat
LOCAL_SRC_FILES = \
	exfat/libexfat/cluster.c \
	exfat/libexfat/io.c \
	exfat/libexfat/log.c \
	exfat/libexfat/lookup.c \
	exfat/libexfat/mount.c \
	exfat/libexfat/node.c \
	exfat/libexfat/repair.c \
	exfat/libexfat/time.c \
	exfat/libexfat/utf.c \
	exfat/libexfat/utils.c 
LOCAL_C_INCLUDES := \
	$(LIBEXFAT_CONFIGURE_DIR)
LOCAL_CFLAGS = $(common_cflags)
include $(BUILD_STATIC_LIBRARY)

###

include $(CLEAR_VARS)
LOCAL_MODULE := libexfat_mount
LOCAL_SRC_FILES = \
	exfat/fuse/main.c
LOCAL_C_INCLUDES += \
	exfat/libexfat \
	fuse/include \
	fuse/android \
	$(LIBEXFAT_CONFIGURE_DIR)
LOCAL_CFLAGS = $(common_cflags)
include $(BUILD_STATIC_LIBRARY)

###

include $(CLEAR_VARS)
LOCAL_MODULE := libexfat_mkfs
LOCAL_SRC_FILES = \
	exfat/mkfs/cbm.c \
	exfat/mkfs/fat.c \
	exfat/mkfs/main.c \
	exfat/mkfs/mkexfat.c \
	exfat/mkfs/rootdir.c \
	exfat/mkfs/uct.c \
	exfat/mkfs/uctc.c \
	exfat/mkfs/vbr.c
LOCAL_C_INCLUDES += \
	exfat/libexfat \
	fuse/include \
	$(LIBEXFAT_CONFIGURE_DIR)
LOCAL_CFLAGS = $(common_cflags)
include $(BUILD_STATIC_LIBRARY)

###

include $(CLEAR_VARS)
LOCAL_MODULE := libexfat_fsck
LOCAL_SRC_FILES = \
	exfat/fsck/main.c
LOCAL_C_INCLUDES += \
	exfat/libexfat \
	fuse/include \
	$(LIBEXFAT_CONFIGURE_DIR)
LOCAL_CFLAGS = $(common_cflags)
include $(BUILD_STATIC_LIBRARY)

###

include $(CLEAR_VARS)
LOCAL_MODULE := mount.exfat
LOCAL_SRC_FILES := \
	exfat/unified.c
LOCAL_STATIC_LIBRARIES := \
	libexfat_mount \
	libexfat_fsck \
	libexfat_mkfs \
	libexfat \
	libfuse
include $(BUILD_EXECUTABLE)

###

include $(CLEAR_VARS)
LOCAL_MODULE := mount.exfat_static
LOCAL_SRC_FILES := \
	exfat/unified.c
LOCAL_STATIC_LIBRARIES := \
	libexfat_mount \
	libexfat_fsck \
	libexfat_mkfs \
	libexfat \
	libfuse
LOCAL_LDFLAGS := -static
include $(BUILD_EXECUTABLE)
