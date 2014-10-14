LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := picprogram
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -std=c99
LOCAL_SRC_FILES := picprogram.c i2c.c hexparse.c bootloadercmds.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := picflashget
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -std=c99
LOCAL_SRC_FILES := picflashget.c i2c.c hexparse.c bootloadercmds.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := pictest
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -std=c99
LOCAL_SRC_FILES := pictest.c i2c.c hexparse.c bootloadercmds.c
include $(BUILD_EXECUTABLE)