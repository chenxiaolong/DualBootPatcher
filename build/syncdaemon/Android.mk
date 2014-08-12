LOCAL_PATH := $(call my-dir)

SYNCDAEMON_VERSION := $(shell source ../compile/env.sh && get_conf builder version)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := syncdaemon.cpp common.cpp configfile.cpp jsoncpp/dist/jsoncpp.cpp
LOCAL_MODULE := syncdaemon
LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES := jsoncpp/dist
# -pedantic
LOCAL_CFLAGS := -Wall -Wextra -fexceptions -DVERSION=\"$(SYNCDAEMON_VERSION)\"
LOCAL_CFLAGS += -fstack-protector-all -D_FORTIFY_SOURCE=2
LOCAL_CFLAGS += -Wl,-z,noexecstack -Wl,-z,now -Wl,-z,relro -pie
LOCAL_LDLIBS := -llog
include $(BUILD_EXECUTABLE)