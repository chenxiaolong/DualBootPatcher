LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := inotify-cxx/inotify-cxx.cpp

LOCAL_MODULE := inotify-cxx
LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES := inotify-cxx

LOCAL_CFLAGS := -Wall -Wextra -fexceptions
LOCAL_CFLAGS += -fstack-protector-all -D_FORTIFY_SOURCE=2
LOCAL_CFLAGS += -Wl,-z,noexecstack -Wl,-z,now -Wl,-z,relro -pie

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	syncdaemon.cpp \
	common.cpp \
	configfile.cpp \
	jsoncpp-dist/jsoncpp.cpp

LOCAL_MODULE := syncdaemon
LOCAL_MODULE_TAGS := eng
LOCAL_C_INCLUDES := \
	jsoncpp-dist \
	inotify-cxx

# -pedantic
LOCAL_CFLAGS := -Wall -Wextra -fexceptions -DVERSION=\"$(SYNCDAEMON_VERSION)\"
LOCAL_CFLAGS += -fstack-protector-all -D_FORTIFY_SOURCE=2
LOCAL_CFLAGS += -Wl,-z,noexecstack -Wl,-z,now -Wl,-z,relro -pie
LOCAL_LDLIBS := -llog

LOCAL_STATIC_LIBRARIES := inotify-cxx

include $(BUILD_EXECUTABLE)
