LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := picprogram
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -std=c99
LOCAL_SRC_FILES := picprogram.c i2c.c
include $(BUILD_EXECUTABLE)

