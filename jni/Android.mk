LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fPIC -std=c99
LOCAL_LDFLAGS += -fPIC 
LOCAL_LDLIBS := -llog

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_MODULE    := mem-tools
LOCAL_SRC_FILES := mem-tools.c

include $(BUILD_EXECUTABLE)