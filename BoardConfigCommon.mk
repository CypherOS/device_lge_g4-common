#
# Copyright (C) 2015 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

BOARD_VENDOR := lge

COMMON_PATH := device/lge/g4-common

TARGET_SPECIFIC_HEADER_PATH := $(COMMON_PATH)/include

include $(COMMON_PATH)/PlatformConfig.mk
include $(COMMON_PATH)/board/*.mk

# HIDL
DEVICE_MANIFEST_FILE := $(COMMON_PATH)/configs/manifest.xml

# Keymaster
TARGET_KEYMASTER_WAIT_FOR_QSEE := true

# Shim libs
TARGET_LD_SHIM_LIBS := \
    /system/vendor/lib/libwvm.so|libshims_wvm.so \
    /system/vendor/lib64/libril-qc-qmi-1.so|/system/lib64/rild_socket.so

# Gestures
TARGET_GESTURES_NODE := "/proc/touchpanel/gesture_enable"
TARGET_TAP_TO_WAKE_NODE := "/proc/touchpanel/double_tap_enable"
TARGET_DRAW_V_NODE := "/proc/touchpanel/down_arrow_enable"
TARGET_DRAW_INVERSE_V_NODE := "/proc/touchpanel/up_arrow_enable"
TARGET_DRAW_O_NODE := "/proc/touchpanel/letter_o_enable"
TARGET_DRAW_ARROW_LEFT_NODE := "/proc/touchpanel/left_arrow_enable"
TARGET_DRAW_ARROW_RIGHT_NODE := "/proc/touchpanel/right_arrow_enable"
TARGET_ONE_FINGER_SWIPE_UP_NODE := "/proc/touchpanel/up_swipe_enable"
TARGET_ONE_FINGER_SWIPE_DOWN_NODE := "/proc/touchpanel/down_swipe_enable"
TARGET_ONE_FINGER_SWIPE_LEFT_NODE := "/proc/touchpanel/left_swipe_enable"
TARGET_ONE_FINGER_SWIPE_RIGHT_NODE := "/proc/touchpanel/right_swipe_enable"
TARGET_TWO_FINGER_SWIPE_NODE := "/proc/touchpanel/double_swipe_enable"

# inherit from the proprietary version
-include vendor/lge/g4-common/BoardConfigVendor.mk
