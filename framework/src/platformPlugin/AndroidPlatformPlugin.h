// Copyright (c) 2017-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

/* This file may have been modified by PICO */

#include "platformPlugin/IXrPlatformPlugin.h"

namespace PVRSampleFW {
    class AndroidPlatformPlugin : public IXrPlatformPlugin {
    public:
        AndroidPlatformPlugin(const std::shared_ptr<struct Configurations>& /*unused*/, void* applicationVM,
                              void* applicationActivity) {
            instance_create_info_android_ = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
            instance_create_info_android_.applicationVM = applicationVM;
            instance_create_info_android_.applicationActivity = applicationActivity;
        }

        XrBaseInStructure* GetInstanceCreateInfo() override {
            return reinterpret_cast<XrBaseInStructure*>((&instance_create_info_android_));
        }

        // OpenXR instance-level extensions required by this platform.
        std::vector<std::string> GetInstanceExtensionsRequiredByPlatform() const override {
            std::vector<std::string> extensions = {XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME};
            return extensions;
        }

        // Perform required steps after updating Configurations
        void UpdateConfigurationsAtPlatform(
                const std::shared_ptr<struct Configurations>& config) override { /* do nothing now */
        }

    private:
        XrInstanceCreateInfoAndroidKHR instance_create_info_android_;
    };
}  // namespace PVRSampleFW
