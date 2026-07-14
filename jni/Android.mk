LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := modmenu
LOCAL_SRC_FILES := src/main.cpp src/il2cpp.cpp src/hooks.cpp src/menu.cpp src/thumbhook.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_CPPFLAGS  := -std=c++17 -O2 -fvisibility=hidden -fno-rtti -fno-exceptions
LOCAL_LDLIBS    := -llog -landroid
LOCAL_ARM_MODE  := thumb

include $(BUILD_SHARED_LIBRARY)
