LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := libmiscstuff.c mntent.c
LOCAL_MODULE := libmiscstuff
LOCAL_MODULE_TAGS := eng
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)
