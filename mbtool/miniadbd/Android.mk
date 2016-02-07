LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libminiadbd
LOCAL_CFLAGS := \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-deprecated-declarations \
	-Wno-missing-field-initializers \
	-Wno-unused-parameter \
	-fvisibility=hidden \
	-D_GNU_SOURCE
LOCAL_SRC_FILES := \
	adb.cpp \
	adb_io.cpp \
	adb_log.cpp \
	adb_utils.cpp \
	fdevent.cpp \
	file_sync_service.cpp \
	services.cpp \
	sockets.cpp \
	transport.cpp \
	transport_usb.cpp \
	usb_linux_client.cpp
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../../libmblog/include \
	$(LOCAL_PATH)/../../libmbutil \
	$(LOCAL_PATH)/../external/linux-api-headers
include $(BUILD_STATIC_LIBRARY)