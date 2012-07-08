# ----------------------------------------------------------------------------
# Makefile for FriendlyARM Mini6410
#

LOCAL_PATH := $(call my-dir)


# ----------------------------------------------------------------------------
# Lets install our own init.rc files :)
include $(CLEAR_VARS)

target_init_rc_file := $(TARGET_ROOT_OUT)/init.rc
$(target_init_rc_file) : $(LOCAL_PATH)/init.rc | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(target_init_rc_file)

target_hw_init_rc_file := $(TARGET_ROOT_OUT)/init.mini6410.rc
$(target_hw_init_rc_file) : $(LOCAL_PATH)/init.mini6410.rc | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(target_hw_init_rc_file)

target_ueventd_rc_file := $(TARGET_ROOT_OUT)/ueventd.rc
$(target_ueventd_rc_file) : $(LOCAL_PATH)/ueventd.rc | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(target_ueventd_rc_file)

$(INSTALLED_RAMDISK_TARGET): $(target_init_rc_file) \
	$(target_hw_init_rc_file) \
	$(target_ueventd_rc_file)

# Initial script hnmsky
file := $(TARGET_OUT)/etc/init.smdk6410.sh
$(file) : $(LOCAL_PATH)/init.smdk6410.sh | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(file)

# proprietary files
ifeq ($(PREBUILT_SO),y)
file := $(TARGET_OUT)/lib/libcamera.so \
	$(TARGET_OUT_INTERMEDIATES)/lib/libcamera.so
$(file) : $(LOCAL_PATH)/proprietary/libcamera.so | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(file)

file := $(TARGET_OUT)/lib/libs3cjpeg.so \
	$(TARGET_OUT_INTERMEDIATES)/lib/libs3cjpeg.so
$(file) : $(LOCAL_PATH)/proprietary/libs3cjpeg.so | $(ACP)
	$(transform-prebuilt-to-target)
ALL_PREBUILT += $(file)
endif


# ----------------------------------------------------------------------------
# copy base files
include $(CLEAR_VARS)

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
#	$(LOCAL_PATH)/asound.conf:system/etc/asound.conf \
#	$(LOCAL_PATH)/wm9714.conf:system/etc/wm9714.conf \
#	$(LOCAL_PATH)/vold.fstab:system/etc/vold.fstab \
#	$(LOCAL_PATH)/tsd:root/sbin/tsd \
#	$(LOCAL_PATH)/friendlyarm-ts-input.conf:system/etc/friendlyarm-ts-input.conf \
#	$(LOCAL_PATH)/linuxrc:root/linuxrc \
	$(LOCAL_PATH)/qwerty.kl:system/usr/keylayout/qwerty.kl


# End of file
# vim: syntax=make

