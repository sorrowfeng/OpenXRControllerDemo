/*
 * Copyright 2024 - 2024 PICO. All rights reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of PICO.
 * The intellectual and technical concepts contained herein are proprietary to PICO.
 * and may be covered by patents, patents in process, and are protected by trade
 * secret or copyright law. Dissemination of this information or reproduction of
 * this material is strictly forbidden unless prior written permission is obtained
 * from PICO.
 */

#ifndef PICONATIVEOPENXRSAMPLES_EXTENSIONFEATURESMANAGER_H
#define PICONATIVEOPENXRSAMPLES_EXTENSIONFEATURESMANAGER_H

#include "IXrPlatformPlugin.h"
#include "IXrGraphicsPlugin.h"
#include "IOpenXRExtensionPlugin.h"
#include "EXTFuture.h"
#include "EXTPerformanceSettings.h"
#include "KHRAndroidThreadSetting.h"
#include "FBDisplayRefreshRates.h"
#include "Passthrough.h"
#include <unordered_map>
#include "BDBodyTracking.h"
#include "PICOBodyTracking.h"
#include "PICOCompositionLayerSetting.h"
#include "FBCompositionLayerSetting.h"

namespace PVRSampleFW {

    // need to be updated when new extension added
    const std::unordered_map<std::string, int> LAZY_REGISTER_SUPPORTED_FEATURES_MAP = {
            {XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME, 0},
            {XR_EXT_FUTURE_EXTENSION_NAME, 1},
            {XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME, 2},
            {XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME, 3},
            {XR_FB_PASSTHROUGH_EXTENSION_NAME, 4},
            {XR_BD_BODY_TRACKING_EXTENSION_NAME, 5},
            {XR_PICO_BODY_TRACKING2_EXTENSION_NAME, 6},
            {XR_FB_COMPOSITION_LAYER_SETTINGS_EXTENSION_NAME, 7},
            {XR_PICO_LAYER_SETTINGS_EXTENSION_NAME, 8}};

    /**
     * @brief ExtensionFeaturesManager is a class used to manage all enable extensions you want.
     *
     * It serves as a component @p BasicOpenXrWrapper.
     */
    class ExtensionFeaturesManager {
    public:
        ExtensionFeaturesManager() = default;

        ~ExtensionFeaturesManager() {
            std::vector<std::shared_ptr<IOpenXRExtensionPlugin>>().swap(extensions_);
        }

        void RegisterExtensionFeature(const std::shared_ptr<IOpenXRExtensionPlugin>& modularFeature);

        // Lazy mode to register extension features.
        bool RegisterExtensionFeatures(const std::vector<std::string>& extensions, BasicOpenXrWrapper* wrapper);

        std::vector<std::shared_ptr<IOpenXRExtensionPlugin>> GetAllRegisteredExtensions() const;

        std::shared_ptr<IOpenXRExtensionPlugin> GetRegisterExtension(const std::string name) const;

        void SetPlatformPlugin(const std::shared_ptr<IXrPlatformPlugin>& plugin) {
            platform_plugin_ = plugin;
        }

        void SetGraphicsPlugin(const std::shared_ptr<IXrGraphicsPlugin>& plugin) {
            graphics_plugin_ = plugin;
        }

    private:
        std::shared_ptr<IXrPlatformPlugin> platform_plugin_;
        std::shared_ptr<IXrGraphicsPlugin> graphics_plugin_;
        std::vector<std::shared_ptr<IOpenXRExtensionPlugin>> extensions_;
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_EXTENSIONFEATURESMANAGER_H
