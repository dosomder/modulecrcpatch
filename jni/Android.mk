LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := modulecrcpatch
LOCAL_SRC_FILES := modulecrcpatch.c

include $(BUILD_EXECUTABLE)
