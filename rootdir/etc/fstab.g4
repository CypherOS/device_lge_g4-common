# Android fstab file.
# The filesystem that contains the filesystem checker binary (typically /system) cannot
# specify MF_CHECK, and must come before any filesystems that do specify MF_CHECK

#TODO: Add 'check' as fs_mgr_flags with data partition.
# Currently we dont have e2fsck compiled. So fs check would failed.

#<src>                                     <mnt_point>     <type>  <mnt_flags and options>                                             <fs_mgr_flags>
/dev/block/bootdevice/by-name/system       /system         ext4    ro,barrier=1,noatime                                                wait,check
/dev/block/bootdevice/by-name/userdata     /data           ext4    nosuid,nodev,barrier=1,noatime,noauto_da_alloc,errors=continue      wait,check,encryptable=/dev/block/bootdevice/by-name/encrypt
/dev/block/bootdevice/by-name/cache        /cache          ext4    nosuid,nodev,barrier=1,noatime,noauto_da_alloc,errors=continue      wait,check
/dev/block/bootdevice/by-name/persist      /persist        ext4    nosuid,nodev,barrier=1,data=ordered,nodelalloc,errors=panic         wait
/dev/block/bootdevice/by-name/persistent   /persistent     emmc    defaults                                                            defaults
/dev/block/bootdevice/by-name/boot         /boot           emmc    defaults                                                            recoveryonly
/dev/block/bootdevice/by-name/recovery     /recovery       emmc    defaults                                                            recoveryonly
/dev/block/bootdevice/by-name/modem        /firmware       vfat    ro,shortname=lower,uid=1000,gid=1000,dmask=227,fmask=337,context=u:object_r:firmware_file:s0        wait

/dev/block/bootdevice/by-name/sns          /sns            ext4    nosuid,nodev,barrier=1,noatime,noauto_da_alloc,errors=continue      wait
/dev/block/bootdevice/by-name/drm          /persist-lg     ext4    nosuid,nodev,barrier=1,noatime,noauto_da_alloc,errors=continue      wait
/dev/block/bootdevice/by-name/mpt          /mpt            ext4    nosuid,nodev,barrier=1,noatime,noauto_da_alloc,errors=continue      wait

/devices/soc.0/f98a4900.sdhci/mmc_host     /storage/external_SD    vfat    nosuid,nodev            wait,voldmanaged=external_SD:auto
/devices/soc.0/f9200000.ssusb/f9200000.dwc3/xhci-hcd.0.auto/usb1/1-1       /storage/USBstorage1    vfat    nosuid,nodev         wait,voldmanaged=USBstorage1:auto
/devices/soc.0/f9200000.ssusb/f9200000.dwc3/xhci-hcd.0.auto/usb1/1-1       /storage/USBstorage2    vfat    nosuid,nodev         wait,voldmanaged=USBstorage2:auto
/devices/soc.0/f9200000.ssusb/f9200000.dwc3/xhci-hcd.0.auto/usb1/1-1       /storage/USBstorage3    vfat    nosuid,nodev         wait,voldmanaged=USBstorage3:auto
/devices/soc.0/f9200000.ssusb/f9200000.dwc3/xhci-hcd.0.auto/usb1/1-1       /storage/USBstorage4    vfat    nosuid,nodev         wait,voldmanaged=USBstorage4:auto
/devices/soc.0/f9200000.ssusb/f9200000.dwc3/xhci-hcd.0.auto/usb1/1-1       /storage/USBstorage5    vfat    nosuid,nodev         wait,voldmanaged=USBstorage5:auto
/devices/soc.0/f9200000.ssusb/f9200000.dwc3/xhci-hcd.0.auto/usb1/1-1       /storage/USBstorage6    vfat    nosuid,nodev         wait,voldmanaged=USBstorage6:auto
