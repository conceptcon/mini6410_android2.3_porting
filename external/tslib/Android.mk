# tslib Android build script
# (c) 2009 Nicu Pavel <npavel@ituner.com>
# tslib looks for these environment variables
# export TSLIB_TSDEVICE=/dev/input/event1
# export TSLIB_CONFFILE=/system/etc/ts.conf
# export TSLIB_CALIBFILE=/etc/pointercal
# export TSLIB_PLUGINDIR=/system/lib
# export TSLIB_CONSOLEDEVICE=/dev/tty
# export TSLIB_FBDEVICE=/dev/graphics/fb0


ifneq ($(TARGET_SIMULATOR),true)
  BUILD_TSLIB := 1
endif
ifeq ($(BUILD_TSLIB),1)

LOCAL_PATH:= $(call my-dir)


# Create a dummy config.h

include $(CLEAR_VARS)
DUMMYHEADER := $(shell echo "" > $(LOCAL_PATH)/config.h)
$(info $(DUMMYHEADER))


#
# Build libts
#

include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/src/

LOCAL_CFLAGS:=-DNO_SHARED_LIBS
LOCAL_CFLAGS+=-DTS_CONF=\"/system/etc/ts.conf\"
LOCAL_CFLAGS+=-DPLUGIN_DIR=\"/system/lib/\"

LOCAL_SRC_FILES:= \
		src/ts_attach.c \
		src/ts_close.c \
		src/ts_config.c \
		src/ts_error.c \
		src/ts_fd.c \
		src/ts_load_module.c \
		src/ts_open.c \
		src/ts_parse_vars.c \
		src/ts_read.c \
		src/ts_read_raw.c \
		src/ts_option.c
#hnmsky
LOCAL_MODULE_TAGS:=eng
LOCAL_MODULE:=libts
#hnmsky
LOCAL_SHARED_LIBRARIES := libdl
include $(BUILD_STATIC_LIBRARY)

#
# dynamic modules
#

# linux input raw module
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/plugins/ \
	$(LOCAL_PATH)/src/

LOCAL_CFLAGS+=-DTS_POINTERCAL=\"/etc/pointercal\"

# Don't add this so to the android prelink map
LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
		plugins/input-raw.c 

LOCAL_MODULE_TAGS:=eng
LOCAL_MODULE:=input

LOCAL_STATIC_LIBRARIES := \
	libts

include $(BUILD_SHARED_LIBRARY)


# variance module
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/plugins/ \
	$(LOCAL_PATH)/src/

LOCAL_CFLAGS+=-DTS_POINTERCAL=\"/etc/pointercal\"

# Don't add this so to the android prelink map
LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
		plugins/variance.c

LOCAL_MODULE_TAGS:=eng
LOCAL_MODULE:=variance

LOCAL_STATIC_LIBRARIES := \
	libts

include $(BUILD_SHARED_LIBRARY)


# dejitter module
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/plugins/ \
	$(LOCAL_PATH)/src/

LOCAL_CFLAGS+=-DTS_POINTERCAL=\"/etc/pointercal\"

# Don't add this so to the android prelink map
LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
		plugins/dejitter.c

LOCAL_MODULE_TAGS:=eng
LOCAL_MODULE:=dejitter

LOCAL_STATIC_LIBRARIES := \
	libts

include $(BUILD_SHARED_LIBRARY)


# linear module
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/plugins/ \
	$(LOCAL_PATH)/src/

LOCAL_CFLAGS+=-DTS_POINTERCAL=\"/etc/pointercal\"

# Don't add this so to the android prelink map
LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
		plugins/linear.c

LOCAL_MODULE_TAGS:=eng
LOCAL_MODULE:=linear

LOCAL_STATIC_LIBRARIES := \
	libts

include $(BUILD_SHARED_LIBRARY)


# pthres module
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/plugins/ \
	$(LOCAL_PATH)/src/

LOCAL_CFLAGS+=-DTS_POINTERCAL=\"/etc/pointercal\"

# Don't add this so to the android prelink map
LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
		plugins/pthres.c

LOCAL_MODULE_TAGS:=eng
LOCAL_MODULE:=pthres

LOCAL_STATIC_LIBRARIES := \
	libts

include $(BUILD_SHARED_LIBRARY)



#
# Build ts_ programs
#

# ts_calibrate
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/tests/ \
	$(LOCAL_PATH)/src/

LOCAL_CFLAGS+=-DTS_POINTERCAL=\"/etc/pointercal\"

LOCAL_SRC_FILES:= \
		tests/fbutils.c \
		tests/font_8x8.c \
		tests/font_8x16 \
		tests/ts_calibrate.c \
		tests/testutils.c 

LOCAL_MODULE_TAGS:=eng
LOCAL_MODULE:=ts_calibrate

LOCAL_STATIC_LIBRARIES := \
	libts
LOCAL_SHARED_LIBRARIES := libdl
include $(BUILD_EXECUTABLE)

# ts_test
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/tests/ \
	$(LOCAL_PATH)/src/

LOCAL_CFLAGS+=-DTS_POINTERCAL=\"/etc/pointercal\"

LOCAL_SRC_FILES:= \
		tests/fbutils.c \
		tests/font_8x8.c \
		tests/font_8x16 \
		tests/ts_test.c 

LOCAL_MODULE_TAGS:=eng
LOCAL_MODULE:=ts_test

LOCAL_STATIC_LIBRARIES := \
	libts
LOCAL_SHARED_LIBRARIES := libdl


include $(BUILD_EXECUTABLE)

# ts_print
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/tests/ \
	$(LOCAL_PATH)/src/

LOCAL_CFLAGS+=-DTS_POINTERCAL=\"/etc/pointercal\"

LOCAL_SRC_FILES:= \
		tests/ts_print.c

LOCAL_MODULE_TAGS:=eng
LOCAL_MODULE:=ts_print

LOCAL_STATIC_LIBRARIES := \
	libts
LOCAL_SHARED_LIBRARIES := libdl
include $(BUILD_EXECUTABLE)


# ts_print_raw
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/tests/ \
	$(LOCAL_PATH)/src/

LOCAL_CFLAGS+=-DTS_POINTERCAL=\"/etc/pointercal\"

LOCAL_SRC_FILES:= \
		tests/ts_print_raw.c

LOCAL_MODULE_TAGS:=eng
LOCAL_MODULE:=ts_print_raw

LOCAL_STATIC_LIBRARIES := \
	libts
LOCAL_SHARED_LIBRARIES := libdl
include $(BUILD_EXECUTABLE)

# ts_harvest
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/tests/ \
	$(LOCAL_PATH)/src/

LOCAL_CFLAGS+=-DTS_POINTERCAL=\"/etc/pointercal\"

LOCAL_SRC_FILES:= \
		tests/fbutils.c \
		tests/font_8x8.c \
		tests/font_8x16 \
		tests/testutils.c \
		tests/ts_harvest.c

LOCAL_MODULE_TAGS:=eng
LOCAL_MODULE:=ts_harvest

LOCAL_STATIC_LIBRARIES := \
	libts
LOCAL_SHARED_LIBRARIES := libdl
include $(BUILD_EXECUTABLE)


# Create a simple  ts.conf configuration file with the linux input module

include $(CLEAR_VARS)
etc-dir := etc
target-etc-dir := $(TARGET_OUT)/etc

TSCONF := $(shell echo "module_raw input" > $(LOCAL_PATH)/$(etc-dir)/ts.conf)
TSCONF += $(shell echo "module pthres pmin=1" >> $(LOCAL_PATH)/$(etc-dir)/ts.conf)
TSCONF += $(shell echo "module variance delta=30" >> $(LOCAL_PATH)/$(etc-dir)/ts.conf)
TSCONF += $(shell echo "module dejitter delta=100" >> $(LOCAL_PATH)/$(etc-dir)/ts.conf)
TSCONF += $(shell echo "module linear" >> $(LOCAL_PATH)/$(etc-dir)/ts.conf)

$(info $(TSCONF))

LOCAL_MODULE := ts.conf
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(target-etc-dir)
LOCAL_SRC_FILES := $(etc-dir)/$(LOCAL_MODULE)

include $(BUILD_PREBUILT)


# ts_thread
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(KERNEL_HEADERS) \
	$(LOCAL_PATH)/tests/ 


LOCAL_CFLAGS+=-DTS_POINTERCAL=\"/etc/pointercal\"

LOCAL_SRC_FILES:= \
		tests/ts_thread.c 

LOCAL_MODULE_TAGS:=eng
LOCAL_MODULE:=ts_thread

#LOCAL_STATIC_LIBRARIES := \
#	libts
#LOCAL_SHARED_LIBRARIES := libdl


include $(BUILD_EXECUTABLE)
endif
