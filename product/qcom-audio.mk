PRODUCT_PACKAGES += \
    android.hardware.audio@2.0-impl \
    android.hardware.audio.effect@2.0-impl \
    android.hardware.soundtrigger@2.0-impl \
    audiod \
    libaudio-resampler \
    libqcompostprocbundle \
    libqcomvisualizer \
    libqcomvoiceprocessing \
    libqcomvoiceprocessingdescriptors \
    libtinyalsa \
    libtinycompress \
    libtinyxml \
    tinymix

# speaker protection
PRODUCT_PROPERTY_OVERRIDES += \
    persist.speaker.prot.enable=false \
    persist.spkr.cal.duration=0

# surround sound recording
PRODUCT_PROPERTY_OVERRIDES += \
    ro.qc.sdk.audio.ssr=false \
    persist.audio.ssr.3mic=true

# offload settings
PRODUCT_PROPERTY_OVERRIDES += \
    av.offload.enable=false \
    audio.offload.buffer.size.kb=32 \
    audio.offload.pcm.16bit.enable=false \
    audio.offload.gapless.enabled=false \
    audio.offload.passthrough=false \
    audio.offload.pcm.24bit.enable=true \
    audio.offload.multiple.enabled=false \
    audio.deep_buffer.media=true \
    tunnel.audio.encode=false \
    use.dedicated.device.for.voip=true

# voip
PRODUCT_PROPERTY_OVERRIDES += \
    use.voice.path.for.pcm.voip=true

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.audio.low_latency.xml:system/etc/permissions/android.hardware.audio.low_latency.xml
