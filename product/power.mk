PRODUCT_PACKAGES += \
    android.hardware.health@1.0-convert \
    android.hardware.health@1.0-impl \
    android.hardware.health@1.0-service \
    android.hardware.power@1.0-impl \
    power.msm8992

PRODUCT_COPY_FILES += \
        $(LOCAL_PATH)/ramdisk/sbin/healthd:root/sbin/healthd \
        $(LOCAL_PATH)/ramdisk/res/images/font_log.png:root/res/images/font_log.png \
        $(LOCAL_PATH)/ramdisk/res/images/charger/cm_battery_scale.png:root/res/images/charger/cm_battery_scale.png \
        $(LOCAL_PATH)/ramdisk/res/images/charger/battery_scale.png:root/res/images/charger/battery_scale.png \
        $(LOCAL_PATH)/ramdisk/res/images/charger/battery_fail.png:root/res/images/charger/battery_fail.png
