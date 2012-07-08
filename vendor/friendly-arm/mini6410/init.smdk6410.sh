#!/system/bin/sh

# disable boot animation for a faster boot sequence when needed
boot_anim=`getprop ro.kernel.android.bootanim`
case "$boot_anim" in
    0)  setprop debug.sf.nobootanimation 1
    ;;
esac

# FA customize
#alsa_ctl restore -f /system/etc/wm9714.conf
#logcat *:V > /data/log.txt &

# Wi-Fi support
MODULE_PATH=/system/lib/modules/`uname -r`
if [ ! -f ${MODULE_PATH}/modules.dep.bb ]; then
	depmod
fi
modprobe libertas_sdio
#modprobe ath9k_htc
#modprobe rt73usb
#modprobe rt2800usb
#modprobe zd1211rw
setprop wlan.driver.status builtin

