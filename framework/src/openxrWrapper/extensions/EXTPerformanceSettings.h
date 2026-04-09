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

#ifndef PICONATIVEOPENXRSAMPLES_EXTPERFORMANCESETTINGS_H
#define PICONATIVEOPENXRSAMPLES_EXTPERFORMANCESETTINGS_H

#include "IOpenXRExtensionPlugin.h"
#include "CheckUtils.h"

namespace PVRSampleFW {

    /**
     * doc: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_performance_settings
     */
    class EXTPerformanceSettings : public IOpenXRExtensionPlugin {
    public:
        EXTPerformanceSettings() {
        }
        EXTPerformanceSettings(const std::shared_ptr<IXrPlatformPlugin>& plaf, BasicOpenXrWrapper* openxrW)
            : IOpenXRExtensionPlugin(plaf, openxrW) {
        }
        virtual ~EXTPerformanceSettings() {
        }

        std::vector<std::string> GetRequiredExtensions() const override;

        bool OnInstanceCreate() override;

        bool OnEventHandlerSetup() override;

        /**
         * Set performance level.
         *
         * @param which type, cpu or gpu
         * @param level level of performance
         * @return 0 is success, others are failed
         */
        int SetPerformanceLevels(XrPerfSettingsDomainEXT which, int level);

        int InitializePerformanceLevels(int cpuLevel, int gpuLevel);

    private:
        XrPerfSettingsLevelEXT ConvertIntLevelToXr(XrPerfSettingsDomainEXT which, int level);

        int ConvertXrLevelToInt(XrPerfSettingsDomainEXT which, XrPerfSettingsLevelEXT level);

    public:
        PFN_DECLARE(xrPerfSettingsSetPerformanceLevelEXT);

    private:
        int perf_setting_cpu_level_{3};
        int perf_setting_gpu_level_{3};
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_EXTPERFORMANCESETTINGS_H
