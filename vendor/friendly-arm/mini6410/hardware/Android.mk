# HAL module implemenation, not prelinked and stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
#LOCAL_C_INCLUDES += 
LOCAL_SHARED_LIBRARIES:= \
	liblog \

LOCAL_SRC_FILES:=               \
   light.c
LOCAL_MODULE_TAGS := eng

LOCAL_MODULE:= lights.default
#.$(TARGET_BOARD_PLATFORM)

#LOCAL_CFLAGS += -fno-short-enums

include $(BUILD_SHARED_LIBRARY)

