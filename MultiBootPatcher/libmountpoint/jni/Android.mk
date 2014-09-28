LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := libmountpoint-jni.cpp libmountpoint.cpp mntent.c
LOCAL_MODULE := libmountpoint-jni
LOCAL_MODULE_TAGS := eng
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)
