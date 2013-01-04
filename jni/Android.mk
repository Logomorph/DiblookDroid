LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := diblook
LOCAL_SRC_FILES := diblook.cpp invert.cpp processing.cpp heap.cpp
LOCAL_LDLIBS    := -lm -llog -ljnigraphics

include $(BUILD_SHARED_LIBRARY)
