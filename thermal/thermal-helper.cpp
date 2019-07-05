/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <set>
#include <sstream>
#include <vector>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include "thermal-helper.h"
#include "utils/ThermalConfigParser.h"

namespace android {
namespace hardware {
namespace thermal {
namespace V1_1 {
namespace implementation {

constexpr char kThermalSensorsRoot[] = "/sys/devices/virtual/thermal";
constexpr char kCpuOnlineRoot[] = "/sys/devices/system/cpu";
constexpr char kCpuUsageFile[] = "/proc/stat";
constexpr char kCpuOnlineFileSuffix[] = "online";
constexpr char kThermalConfigPrefix[] = "/vendor/etc/thermal-engine-";
constexpr char kLittleCoreCpuFreq[] = "thermal-cpufreq-0";
constexpr char kBigCoreCpuFreq[] = "thermal-cpufreq-6";
constexpr char kUsbCdevName[] = "usb";
constexpr char kUsbSensorType[] = "usbc-therm-adc";
constexpr unsigned int kMaxCpus = 8;
// The number of available sensors in thermalHAL is:
// 8 (for each cpu) + 2 (for each gpu) + battery + skin + usb = 13.
constexpr unsigned int kAvailableSensors = 14;
// The following constants are used for limiting the number of throttling
// notifications. See b/117438310 for details.
constexpr int kDesiredLittleCoreCoolingStateCliff = 5;
constexpr int kDesiredBigCoreCoolingStateCliff = 7;

// This is a golden set of thermal sensor name and releveant information about
// the sensor. Used when we read in sensor values.
const std::map<std::string, SensorInfo> kValidThermalSensorInfoMap = {
    {"cpu0-silver-usr", {TemperatureType::CPU, true, 95.0, 125.0, .001}},  // CPU0
    {"cpu1-silver-usr", {TemperatureType::CPU, true, 95.0, 125.0, .001}},  // CPU1
    {"cpu2-silver-usr", {TemperatureType::CPU, true, 95.0, 125.0, .001}},  // CPU2
    {"cpu3-silver-usr", {TemperatureType::CPU, true, 95.0, 125.0, .001}},  // CPU3
    {"cpu4-silver-usr", {TemperatureType::CPU, true, 95.0, 125.0, .001}},  // CPU4
    {"cpu5-silver-usr", {TemperatureType::CPU, true, 95.0, 125.0, .001}},  // CPU5
    {"cpu0-gold-usr", {TemperatureType::CPU, true, 95.0, 125.0, .001}},    // CPU6
    {"cpu1-gold-usr", {TemperatureType::CPU, true, 95.0, 125.0, .001}},    // CPU7
    // GPU thermal sensors.
    {"gpu0-usr", {TemperatureType::GPU, true, 95.0, 125.0, .001}},
    {"gpu1-usr", {TemperatureType::GPU, true, 95.0, 125.0, .001}},
    // Battery thermal sensor.
    {"battery", {TemperatureType::BATTERY, true, NAN, 60.0, .001}},
    // Skin sensor.
    {kSkinSensorType, {TemperatureType::SKIN, false, NAN, NAN, .001}},
    // USBC thermal sensor.
    {kUsbSensorType, {TemperatureType::SKIN, false, 63, NAN, .001}},
    {"pa-therm0-adc", {TemperatureType::UNKNOWN, false, NAN, NAN, .001}},
};

namespace {

using android::base::StringPrintf;

void parseCpuUsagesFileAndAssignUsages(hidl_vec<CpuUsage> *cpu_usages) {
    uint64_t cpu_num, user, nice, system, idle;
    std::string cpu_name;
    std::string data;
    if (!android::base::ReadFileToString(kCpuUsageFile, &data)) {
        LOG(ERROR) << "Error reading Cpu usage file: " << kCpuUsageFile;
        return;
    }

    std::istringstream stat_data(data);
    std::string line;
    while (std::getline(stat_data, line)) {
        if (line.find("cpu") == 0 && isdigit(line[3])) {
            // Split the string using spaces.
            std::vector<std::string> words = android::base::Split(line, " ");
            cpu_name = words[0];
            cpu_num = std::stoi(cpu_name.substr(3));

            if (cpu_num < kMaxCpus) {
                user = std::stoi(words[1]);
                nice = std::stoi(words[2]);
                system = std::stoi(words[3]);
                idle = std::stoi(words[4]);

                // Check if the CPU is online by reading the online file.
                std::string cpu_online_path = StringPrintf("%s/%s/%s", kCpuOnlineRoot,
                                                           cpu_name.c_str(), kCpuOnlineFileSuffix);
                std::string is_online;
                if (!android::base::ReadFileToString(cpu_online_path, &is_online)) {
                    LOG(ERROR) << "Could not open Cpu online file: " << cpu_online_path;
                    return;
                }
                is_online = android::base::Trim(is_online);

                (*cpu_usages)[cpu_num].name = cpu_name;
                (*cpu_usages)[cpu_num].active = user + nice + system;
                (*cpu_usages)[cpu_num].total = user + nice + system + idle;
                (*cpu_usages)[cpu_num].isOnline = (is_online == "1") ? true : false;
            } else {
                LOG(ERROR) << "Unexpected cpu number: " << words[0];
                return;
            }
        }
    }
}

float getThresholdFromType(const TemperatureType type, const ThrottlingThresholds &threshold) {
    switch (type) {
        case TemperatureType::CPU:
            return threshold.cpu;
        case TemperatureType::GPU:
            return threshold.gpu;
        case TemperatureType::BATTERY:
            return threshold.battery;
        case TemperatureType::SKIN:
            return threshold.ss;
        default:
            return NAN;
    }
}

}  // namespace

// This is a golden set of cooling device types and their corresponding sensor
// thernal zone name.
static const std::map<std::string, std::string> kValidCoolingDeviceTypeMap = {
    {kLittleCoreCpuFreq, "cpu0-silver-usr"},  // CPU0
    {kBigCoreCpuFreq, "cpu0-gold-usr"},       // CPU6
    {kUsbCdevName, kUsbSensorType}, // USB connector
};

void ThermalHelper::updateOverideThresholds() {
    for (const auto &sensorMap : kValidThermalSensorInfoMap) {
        if (sensorMap.second.is_override) {
            switch (sensorMap.second.type) {
                case TemperatureType::CPU:
                    thresholds_.cpu = sensorMap.second.throttling;
                    vr_thresholds_.cpu = sensorMap.second.throttling;
                    shutdown_thresholds_.cpu = sensorMap.second.shutdown;
                    break;
                case TemperatureType::GPU:
                    thresholds_.gpu = sensorMap.second.throttling;
                    vr_thresholds_.gpu = sensorMap.second.throttling;
                    shutdown_thresholds_.gpu = sensorMap.second.shutdown;
                    break;
                case TemperatureType::BATTERY:
                    thresholds_.battery = sensorMap.second.throttling;
                    vr_thresholds_.battery = sensorMap.second.throttling;
                    shutdown_thresholds_.battery = sensorMap.second.shutdown;
                    break;
                case TemperatureType::SKIN:
                    thresholds_.ss = sensorMap.second.throttling;
                    vr_thresholds_.ss = sensorMap.second.throttling;
                    shutdown_thresholds_.ss = sensorMap.second.shutdown;
                    break;
                default:
                    break;
            }
        }
    }
}

/*
 * Populate the sensor_name_to_file_map_ map by walking through the file tree,
 * reading the type file and assigning the temp file path to the map.  If we do
 * not succeed, abort.
 */
ThermalHelper::ThermalHelper()
    : is_initialized_(initializeSensorMap() && initializeCoolingDevices()) {
    if (!is_initialized_) {
        LOG(FATAL) << "ThermalHAL could not be initialized properly.";
    }

    std::string hw = android::base::GetProperty("ro.hardware", "");
    std::string thermal_config(kThermalConfigPrefix + hw + ".conf");
    std::string vr_thermal_config(kThermalConfigPrefix + hw + "-vr.conf");
    InitializeThresholdsFromThermalConfig(thermal_config, vr_thermal_config,
                                          kValidThermalSensorInfoMap, &thresholds_,
                                          &shutdown_thresholds_, &vr_thresholds_);
    updateOverideThresholds();
}

std::vector<std::string> ThermalHelper::getCoolingDevicePaths() {
    std::vector<std::string> paths;
    for (const auto &entry : kValidCoolingDeviceTypeMap) {
        std::string path = cooling_devices_.getCoolingDevicePath(entry.first);
        if (!path.empty()) {
            paths.push_back(path + "/cur_state");
        }
    }
    return paths;
}

const std::map<std::string, std::string> &ThermalHelper::getValidCoolingDeviceMap() const {
    return kValidCoolingDeviceTypeMap;
}

bool ThermalHelper::readCoolingDevice(const std::string &cooling_device, int *data) const {
    return cooling_devices_.getCoolingDeviceState(cooling_device, data);
}

bool ThermalHelper::readTemperature(const std::string &sensor_name, Temperature *out) const {
    // Read the file.  If the file can't be read temp will be empty string.
    std::string temp;
    std::string path;

    if (!thermal_sensors_.readSensorFile(sensor_name, &temp, &path)) {
        LOG(ERROR) << "readTemperature: sensor not found: " << sensor_name;
        return false;
    }

    if (temp.empty() && !path.empty()) {
        LOG(ERROR) << "readTemperature: failed to open file: " << path;
        return false;
    }

    SensorInfo sensor_info = kValidThermalSensorInfoMap.at(sensor_name);

    out->type = sensor_info.type;
    out->name = sensor_name;
    out->currentValue = std::stoi(temp) * sensor_info.multiplier;

    if (sensor_name == kUsbSensorType) {
        out->throttlingThreshold = sensor_info.throttling;
        out->shutdownThreshold = sensor_info.shutdown;
        out->vrThrottlingThreshold = sensor_info.throttling;
    } else {
        out->throttlingThreshold = getThresholdFromType(sensor_info.type, thresholds_);
        out->shutdownThreshold = getThresholdFromType(sensor_info.type, shutdown_thresholds_);
        out->vrThrottlingThreshold = getThresholdFromType(sensor_info.type, vr_thresholds_);
    }

    LOG(DEBUG) << StringPrintf("readTemperature: %d, %s, %g, %g, %g, %g", out->type,
                               out->name.c_str(), out->currentValue, out->throttlingThreshold,
                               out->shutdownThreshold, out->vrThrottlingThreshold);

    return true;
}

bool ThermalHelper::initializeSensorMap() {
    for (const auto& sensor_info : kValidThermalSensorInfoMap) {
        std::string sensor_name = sensor_info.first;
        std::string sensor_temp_path = StringPrintf(
            "%s/tz-by-name/%s/temp", kThermalSensorsRoot, sensor_name.c_str());
        if (!thermal_sensors_.addSensor(sensor_name, sensor_temp_path)) {
            LOG(ERROR) << "Could not add " << sensor_name << "to sensors map";
        }
    }
    if (kAvailableSensors == thermal_sensors_.getNumSensors() ||
        kValidThermalSensorInfoMap.size() == thermal_sensors_.getNumSensors()) {
        return true;
    }
    return false;
}

bool ThermalHelper::initializeCoolingDevices() {
    for (const auto& cooling_device_info : kValidCoolingDeviceTypeMap) {
        std::string cooling_device_name = cooling_device_info.first;
        std::string cooling_device_path = StringPrintf(
            "%s/cdev-by-name/%s", kThermalSensorsRoot,
            cooling_device_name.c_str());

        if (!cooling_devices_.addCoolingDevice(
                cooling_device_name, cooling_device_path)) {
            LOG(ERROR) << "Could not add " << cooling_device_name
                       << "to cooling device map";
            continue;
        }

        int data;
        if (cooling_devices_.getCoolingDeviceState(
                cooling_device_name, &data)) {
            cooling_device_path_to_throttling_level_map_.emplace(
                cooling_devices_.getCoolingDevicePath(
                    cooling_device_name).append("/cur_state"),
                data);
        } else {
            LOG(ERROR) << "Could not read cooling device value.";
        }
    }

    if (kValidCoolingDeviceTypeMap.size() ==
            cooling_devices_.getNumCoolingDevices()) {
        return true;
    }
    return false;
}

bool ThermalHelper::fillTemperatures(hidl_vec<Temperature> *temperatures) {
    temperatures->resize(kAvailableSensors);
    int current_index = 0;
    for (const auto &name_type_pair : kValidThermalSensorInfoMap) {
        Temperature temp;

        if (readTemperature(name_type_pair.first, &temp)) {
            (*temperatures)[current_index] = temp;
        } else {
            LOG(ERROR) << "Error reading temperature for sensor: " << name_type_pair.first;
            return false;
        }
        ++current_index;
    }
    return current_index > 0;
}

bool ThermalHelper::fillCpuUsages(hidl_vec<CpuUsage> *cpu_usages) {
    cpu_usages->resize(kMaxCpus);
    parseCpuUsagesFileAndAssignUsages(cpu_usages);
    return true;
}

int ThermalHelper::getMaxThrottlingLevelFromMap() const {
    auto max_element = std::max_element(
        cooling_device_path_to_throttling_level_map_.begin(),
        cooling_device_path_to_throttling_level_map_.end(),
        [](const std::pair<std::string, int> &p1, const std::pair<std::string, int> &p2) {
            return p1.second < p2.second;
        });
    return max_element->second;
}

bool ThermalHelper::checkThrottlingData(const std::pair<std::string, std::string> &throttling_data,
                                        std::pair<bool, Temperature> *notify_params) {
    Temperature temp;

    // If throttling data is in the map add it into the map and check the
    // conditions for notification. If not just check if we're alreadhy
    // throttling or not and notify.
    std::string cooling_device = throttling_data.first;
    if (!cooling_device.empty() &&
        cooling_device_path_to_throttling_level_map_.find(cooling_device) !=
            cooling_device_path_to_throttling_level_map_.end()) {
        int throttling_level = std::stoi(throttling_data.second);
        int max_throttling_level = getMaxThrottlingLevelFromMap();

        // Identify if cooling device is triggered by usb mitigation.
        std::string usb_cdev_path =
                cooling_devices_.getCoolingDevicePath(kUsbCdevName) + "/cur_state";

        if (cooling_device == usb_cdev_path) {
            if (!readTemperature(kUsbSensorType, &temp)) {
                LOG(ERROR) << "Could not read USBC sensor temperature.";
                return false;
            }
            std::string usb_cdev_max_path =
                cooling_devices_.getCoolingDevicePath(kUsbCdevName) + "/max_state";
            std::string usb_cdev_max_state;
            if (!android::base::ReadFileToString(usb_cdev_max_path, &usb_cdev_max_state)) {
                LOG(ERROR) << "Could not read USB CDEV max state";
                return false;
            }

	    // Only trigger notification when usb cdev state is max or clear
            if (throttling_level == std::stoi(usb_cdev_max_state)) {
                *notify_params = std::make_pair(true, temp);
                return true;
            } else if (throttling_level == 0) {
                *notify_params = std::make_pair(false, temp);
                return true;
            } else {
                return false;
	    }
        }

        if (!readTemperature(kSkinSensorType, &temp)) {
            LOG(ERROR) << "Could not read skin sensor temperature.";
            return false;
        }

        // The following if-else blocks aim to reduce the number of notifications
        // triggered by low-level throttling states. See b/117438310 for details.
        if (throttling_level) {
            std::string little_cd_path =
                cooling_devices_.getCoolingDevicePath(kLittleCoreCpuFreq) + "/cur_state";
            std::string big_cd_path =
                cooling_devices_.getCoolingDevicePath(kBigCoreCpuFreq) + "/cur_state";
            if ((cooling_device == little_cd_path &&
                 throttling_level < kDesiredLittleCoreCoolingStateCliff) ||
                (cooling_device == big_cd_path &&
                 throttling_level < kDesiredBigCoreCoolingStateCliff)) {
                LOG(INFO) << "Masking throttling level " << throttling_level << " for CD "
                          << cooling_device;
                throttling_level = 0;
            }
        }

        cooling_device_path_to_throttling_level_map_[throttling_data.first] = throttling_level;

        // We only want to send throttling notifications whenever a new
        // throttling level is reached or if we stop throttling. This first case
        // is to check if a CPU has been throttled higher than the current max.
        // This means that we have to notify throttling and set is_throttling to
        // true. The second case is to check that we are no longer throttling.
        // Meaning that we notify throttling and set is_throttling to false.
        if (max_throttling_level < throttling_level) {
            *notify_params = std::make_pair(true, temp);
            return true;
        } else if (max_throttling_level != 0 && getMaxThrottlingLevelFromMap() == 0) {
            *notify_params = std::make_pair(false, temp);
            return true;
        }
    }

    return false;
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace thermal
}  // namespace hardware
}  // namespace android
