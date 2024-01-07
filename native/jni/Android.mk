LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := trace-init
LOCAL_SRC_FILES := main.cpp trace-init.cpp ptrace-utils.cpp trace-exec-inject.cpp
LOCAL_STATIC_LIBRARIES := libcxx
LOCAL_LDLIBS := -llog
include $(BUILD_EXECUTABLE)

include jni/libcxx/Android.mk
