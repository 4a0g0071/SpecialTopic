LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include E:\NVPACK\OpenCV-2.4.5-Tegra-sdk-r2\sdk\native\jni\OpenCV.mk

LOCAL_MODULE    := mixed_sample
LOCAL_SRC_FILES := jni_part.cpp
LOCAL_LDLIBS +=  -llog -ldl

include $(BUILD_SHARED_LIBRARY)
