# This is a generic product that isn't specialized for a specific device.
# It includes the base Android platform. If you need Google-specific features,
# you should derive from generic_with_google.mk

PRODUCT_PACKAGES := \
    AccountAndSyncSettings \
    AlarmClock \
    AlarmProvider \
    Bluetooth \
    Calculator \
    Calendar \
    Camera \
    CertInstaller \
    DrmProvider \
    Email \
    Gallery \
    LatinIME \
    Mms \
    Music \
    Provision \
    Settings \
    Sync \
    Updater \
    CalendarProvider \
    SyncProvider \
	LiveWallpapersPicker \
	ApiDemos \
	SoftKeyboard \
	CubeLiveWallpapers

$(call inherit-product, build/target/product/core.mk)

# Overrides
PRODUCT_MANUFACTURER	:= FriendlyARM
PRODUCT_BRAND	:= FriendlyARM
PRODUCT_NAME	:= mini6410
PRODUCT_DEVICE := mini6410
PRODUCT_LOCALES	:= en_US zh_CN
