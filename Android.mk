LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := blueping
LOCAL_SRC_FILES := src/main.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_CFLAGS := -std=c11 -Wall -Wextra -Werror -Wunused-parameter -Wno-unused-parameter

LOCAL_LDLIBS := -llog  # Link to Android logging library

include $(BUILD_EXECUTABLE)
