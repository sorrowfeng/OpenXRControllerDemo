/*
 * Copyright 2025 - 2025 PICO. All rights reserved.  
 *
 * NOTICE: All information contained herein is, and remains the property of PICO.
 * The intellectual and technical concepts contained herein are proprietary to PICO.
 * and may be covered by patents, patents in process, and are protected by trade
 * secret or copyright law. Dissemination of this information or reproduction of
 * this material is strictly forbidden unless prior written permission is obtained
 * from PICO.
 */

#include "PICOCompositionLayerSetting.h"

namespace PVRSampleFW {
    std::vector<std::string> PICOCompositionLayerSetting::GetRequiredExtensions() const {
        return {XR_PICO_LAYER_SETTINGS_EXTENSION_NAME};
    }

    bool PICOCompositionLayerSetting::SetLayerSettingFlag(XrCompositionLayerBaseHeader *layer, XrFlags64 flag) {
        // check non-nullptr
        if (layer == nullptr) {
            PLOGE("PICOCompositionLayerSetting::SetLayerSettingFlag failed for layer is nullptr");
            return false;
        }

        layer_setting_map_[layer].type = XR_TYPE_LAYER_SETTINGS_PICO;
        layer_setting_map_[layer].layerFlags |= flag;
        return true;
    }

    bool PICOCompositionLayerSetting::ClearLayerSettingFlag(XrCompositionLayerBaseHeader *layer, XrFlags64 flag) {
        // check non-nullptr
        if (layer == nullptr) {
            PLOGE("PICOCompositionLayerSetting::ClearLayerSettingFlag failed for layer is nullptr");
            return false;
        }

        layer_setting_map_[layer].type = XR_TYPE_UNKNOWN;
        layer_setting_map_[layer].layerFlags &= ~flag;
        return true;
    }

    bool PICOCompositionLayerSetting::EnableLayerSettingFlagToAllLayer(XrFlags64 flag) {
        global_layer_setting_.type = XR_TYPE_LAYER_SETTINGS_PICO;
        global_layer_setting_.layerFlags |= flag;
        if (!is_global_flag_enable_) {
            is_global_flag_enable_ = true;
        }
        return true;
    }

    bool PICOCompositionLayerSetting::DisableLayerSettingFlagToAllLayer(XrFlags64 flag) {
        if (!is_global_flag_enable_) {
            return false;
        }
        global_layer_setting_.type = XR_TYPE_UNKNOWN;
        global_layer_setting_.layerFlags &= ~flag;
        return true;
    }

    bool PICOCompositionLayerSetting::OnPreEndFrame(std::vector<XrCompositionLayerBaseHeader *> *layers) {
        for (auto &layer : *layers) {
            // check non-nullptr
            if (layer == nullptr) {
                continue;
            }
            // find layer in map
            auto iter = layer_setting_map_.find(layer);
            if (iter == layer_setting_map_.end()) {
                if (is_global_flag_enable_) {
                    CheckAndExtendCompositionLayer(layer, &global_layer_setting_);
                }
                continue;
            }
            CheckAndExtendCompositionLayer(layer, &iter->second);
        }
        return true;
    }
}  // namespace PVRSampleFW
