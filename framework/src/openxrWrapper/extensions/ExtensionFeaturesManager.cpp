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

#include "ExtensionFeaturesManager.h"
#include "BasicOpenXrWrapper.h"
#include "ExceptionUtils.h"

namespace PVRSampleFW {
    void
    ExtensionFeaturesManager::RegisterExtensionFeature(const std::shared_ptr<IOpenXRExtensionPlugin> &modularFeature) {
        extensions_.push_back(modularFeature);
    }

    std::vector<std::shared_ptr<IOpenXRExtensionPlugin>> ExtensionFeaturesManager::GetAllRegisteredExtensions() const {
        return extensions_;
    }

    bool ExtensionFeaturesManager::RegisterExtensionFeatures(const std::vector<std::string> &extensions,
                                                             BasicOpenXrWrapper *wrapper) {
        if (extensions.empty()) {
            PLOGW("ExtensionFeaturesManager::RegisterExtensionFeatures extensions is empty");
            return true;
        }
        for (auto &extension : extensions) {
            // check if runtime support the extension
            if (!wrapper->IsExtensionSupported(extension.c_str())) {
                PLOGW("ExtensionFeaturesManager::RegisterExtensionFeatures extension %s is not supported",
                      extension.c_str());
                throw ExtensionNotSupportedException(extension);
                return false;
            }
            auto it = LAZY_REGISTER_SUPPORTED_FEATURES_MAP.find(extension);
            if (it == LAZY_REGISTER_SUPPORTED_FEATURES_MAP.end()) {
                PLOGE("ExtensionFeaturesManager::RegisterExtensionFeatures extension %s is not found",
                      extension.c_str());
                return false;
            } else {
                auto featureEnum = it->second;
                switch (featureEnum) {
                case 0: {
                    auto ext = std::make_shared<KHRAndroidThreadSetting>();
                    ext->SetOpenXrWrapper(wrapper);
                    ext->SetPlatformPlugin(platform_plugin_);
                    ext->SetEnable(true);
                    extensions_.push_back(ext);
                    break;
                }
                case 1: {
                    auto ext = std::make_shared<EXTFuture>();
                    ext->SetOpenXrWrapper(wrapper);
                    ext->SetPlatformPlugin(platform_plugin_);
                    ext->SetEnable(true);
                    extensions_.push_back(ext);
                    break;
                }
                case 2: {
                    auto ext = std::make_shared<EXTPerformanceSettings>();
                    ext->SetOpenXrWrapper(wrapper);
                    ext->SetPlatformPlugin(platform_plugin_);
                    ext->SetEnable(true);
                    extensions_.push_back(ext);
                    break;
                }
                case 3: {
                    auto ext = std::make_shared<FBDisplayRefreshRates>();
                    ext->SetOpenXrWrapper(wrapper);
                    ext->SetPlatformPlugin(platform_plugin_);
                    ext->SetEnable(true);
                    auto target_display_refresh_rate = wrapper->GetTargetDispalyRefreshRate();
                    ext->SetConfiguredRefreshRate(target_display_refresh_rate);
                    extensions_.push_back(ext);
                    break;
                }
                case 4: {
                    auto ext = std::make_shared<Passthrough>();
                    ext->SetOpenXrWrapper(wrapper);
                    ext->SetPlatformPlugin(platform_plugin_);
                    ext->SetEnable(true);
                    extensions_.push_back(ext);
                    break;
                }
                case 5: {
                    auto ext = std::make_shared<BDBodyTracking>();
                    ext->SetOpenXrWrapper(wrapper);
                    ext->SetPlatformPlugin(platform_plugin_);
                    ext->SetEnable(true);
                    extensions_.push_back(ext);
                    break;
                }
                case 6: {
                    auto ext = std::make_shared<PICOBodyTracking>();
                    ext->SetOpenXrWrapper(wrapper);
                    ext->SetPlatformPlugin(platform_plugin_);
                    ext->SetEnable(true);
                    extensions_.push_back(ext);
                    break;
                }
                case 7: {
                    auto ext = std::make_shared<FBCompositionLayerSetting>();
                    ext->SetOpenXrWrapper(wrapper);
                    ext->SetPlatformPlugin(platform_plugin_);
                    ext->SetEnable(true);
                    extensions_.push_back(ext);
                    break;
                }
                case 8: {
                    auto ext = std::make_shared<PICOCompositionLayerSetting>();
                    ext->SetOpenXrWrapper(wrapper);
                    ext->SetPlatformPlugin(platform_plugin_);
                    ext->SetEnable(true);
                    extensions_.push_back(ext);
                    break;
                }
                default:
                    PLOGE("ExtensionFeaturesManager::RegisterExtensionFeatures extension %s is not supported",
                          extension.c_str());
                    return false;
                }
            }
        }
        return true;
    }

    std::shared_ptr<IOpenXRExtensionPlugin>
    ExtensionFeaturesManager::GetRegisterExtension(const std::string name) const {
        // traverse extensions_ to find the extension
        for (auto &extension : extensions_) {
            auto extNames = extension->GetRequiredExtensions();
            for (auto &extName : extNames) {
                if (extName == name) {
                    return extension;
                }
            }
        }
        PLOGE("ExtensionFeaturesManager::GetRegisterExtension extension %s is not found", name.c_str());
        return nullptr;
    }

    /*void
    ExtensionFeaturesManager::SetPlatformPlugin(const std::shared_ptr<IXrPlatformPlugin>& pp) {
        platformPlugin = pp;
        for (auto& extension : extensions_) {
            extension->SetPlatformPlugin(pp);
        }
    }

    void ExtensionFeaturesManager::SetOpenXrWrapper(BasicOpenXrWrapper* PP) {
        for (auto& extension : extensions_) {
            extension->SetOpenXrWrapper(PP);
        }
    }*/
}  // namespace PVRSampleFW
