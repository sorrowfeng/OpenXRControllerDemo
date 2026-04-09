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

#ifndef PICONATIVEOPENXRSAMPLES_PICOCOMPOSITIONLAYERSETTING_H
#define PICONATIVEOPENXRSAMPLES_PICOCOMPOSITIONLAYERSETTING_H

#include "IXrPlatformPlugin.h"
#include "openxr/openxr.h"
#include "IOpenXRExtensionPlugin.h"
#include "CheckUtils.h"
#include <atomic>
#include <unordered_map>

namespace PVRSampleFW {

    class PICOCompositionLayerSetting : public IOpenXRExtensionPlugin {
    public:
        PICOCompositionLayerSetting() = default;
        ~PICOCompositionLayerSetting() = default;

        std::vector<std::string> GetRequiredExtensions() const override;

        bool OnPreEndFrame(std::vector<XrCompositionLayerBaseHeader*>* layers) override;

        /// @note flag set will override the global flag to all layers
        bool SetLayerSettingFlag(XrCompositionLayerBaseHeader* layer, XrFlags64 flag);

        /// @note flag clear will override the global flag to all layers
        bool ClearLayerSettingFlag(XrCompositionLayerBaseHeader* layer, XrFlags64 flag);

        bool EnableLayerSettingFlagToAllLayer(XrFlags64 flag);

        bool DisableLayerSettingFlagToAllLayer(XrFlags64 flag);

    private:
        std::unordered_map<XrCompositionLayerBaseHeader*, XrLayerSettingsPICO> layer_setting_map_;
        XrLayerSettingsPICO global_layer_setting_{XR_TYPE_LAYER_SETTINGS_PICO};
        std::atomic_bool is_global_flag_enable_{false};
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_PICOCOMPOSITIONLAYERSETTING_H
