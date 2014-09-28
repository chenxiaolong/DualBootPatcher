LOCAL_PATH := $(call my-dir)

# libloki JNI
include $(CLEAR_VARS)
LOCAL_SRC_FILES := loki_flash.c loki_patch.c loki_find.c loki_unlok.c loki_jni.c
LOCAL_MODULE := libloki-jni
LOCAL_MODULE_TAGS := eng
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)
