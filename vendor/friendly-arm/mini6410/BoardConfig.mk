# config.mk
#
# Product-specific compile-time definitions.
#

# The generic product target doesn't have any hardware-specific pieces.
#TARGET_ARCH_VARIANT := armv6-vfp
TARGET_CPU_ABI := armeabi
TARGET_NO_BOOTLOADER := true
TARGET_NO_KERNEL := true
TARGET_PROVIDES_INIT_RC := true
TARGET_PROVIDES_UEVENTD_RC := true

#BOARD_NO_RGBX_8888 := true
#BOARD_HAS_LIMITED_EGL := true
## JS Engine
#JS_ENGINE=jsc
#ENABLE_JSC_JIT=true
#BOARD_USES_OVERLAY := true
#DEFAULT_FB_NUM := 4
# Customized map
#TARGET_PRELINKER_MAP := vendor/friendly-arm/products/prelink-linux-arm.map

# Hardware 3D
TARGET_HARDWARE_3D := false

# Audio
#BOARD_USES_ALSA_AUDIO := true
#BUILD_WITH_ALSA_UTILS := true
BOARD_USES_GENERIC_AUDIO := true
# Camera
BOARD_CAMERA_LIBRARIES := libcamera
BOARD_S3CJPEG_LIBRARIES := libs3cjpeg
#USE_CAMERA_STUB := true#hnmsky
# Wi-Fi
#ture
BOARD_HAVE_LIBWIFI := true
BOARD_USES_REALTEK_WIFI := true
BOARD_WPA_SUPPLICANT_DRIVER := WEXT
WIFI_DRIVER_MODULE_PATH := /system/lib/modules/2.6.36mini6410\+/libertas_sdio.ko
WIFI_DRIVER_MODULE_NAME := wlan0
#WIFI_DRIVER_MODULE_ARG
#CUSTOM
######CONFIG_DRIVER_NL80211 := true
WPA_BUILD_SUPPLICANT := true
#WPA_SUPPLICANT_VERSION := VER_0_6_X
WPA_SUPPLICANT_VERSION := VER_0_5_X
CONFIG_CTRL_IFACE := y
#WIFI_DRIVER_MODULE_PATH :=
#WIFI_DRIVER_MODULE_NAME:= wlan0
#WIFI_DRIVER_MODULE_ARG:=

# Bluetooth
#true
BOARD_HAVE_BLUETOOTH := false

# GPS
#BOARD_GPS_LIBRARIES :=libgps

# Media
#BUILD_WITHOUT_PV := true
#BUILD_WITH_FULL_STAGEFRIGHT := true
