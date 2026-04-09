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

#include "EXTPerformanceSettings.h"
#include "BasicOpenXrWrapper.h"

namespace PVRSampleFW {
    std::vector<std::string> EXTPerformanceSettings::GetRequiredExtensions() const {
        return {XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME};
    }

    bool EXTPerformanceSettings::OnInstanceCreate() {
        // Initialize the function pointers
        if (openxr_wrapper_ == nullptr) {
            PLOGE("EXTPerformanceSettings::OnInstanceCreate failed for openxr_wrapper_ is null");
            return false;
        }

        CHECK_XRCMD(
                xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrPerfSettingsSetPerformanceLevelEXT",
                                      reinterpret_cast<PFN_xrVoidFunction *>(&xrPerfSettingsSetPerformanceLevelEXT)));

        return true;
    }

    bool EXTPerformanceSettings::OnEventHandlerSetup() {
        /// Here is a demonstration of handling a generic xrEvent.
        if (openxr_wrapper_ == nullptr) {
            PLOGE("EXTPerformanceSettings::OnEventHandlerSetup failed for openxr_wrapper_ is null");
            return false;
        }
        // Register the event handler
        XrEventHandler eventHandler = {
                .event_type = XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED,
                .handler =
                        [this](BasicOpenXrWrapper *userData, const XrEventDataBaseHeader *eventData,
                               bool *exitRenderLoop, bool *requestRestart) {
                            BasicOpenXrWrapper *openxr = userData;
                            const XrEventDataSessionStateChanged *const sessionStateChanged =
                                    reinterpret_cast<const XrEventDataSessionStateChanged *>(eventData);
                            XrSessionState state = sessionStateChanged->state;

                            if (state == XR_SESSION_STATE_READY) {
                                if (openxr->IsSessionRunning()) {
                                    PLOGI("EXTPerformanceSettings::OnEventHandlerSetup XR_SESSION_STATE_READY");
                                    // Set the performance level
                                    this->SetPerformanceLevels(XR_PERF_SETTINGS_DOMAIN_CPU_EXT,
                                                               perf_setting_cpu_level_);
                                    this->SetPerformanceLevels(XR_PERF_SETTINGS_DOMAIN_GPU_EXT,
                                                               perf_setting_gpu_level_);
                                } else {
                                    PLOGW("EXTPerformanceSettings::eventHandler failed, session not running");
                                }
                            }
                        },
        };

        openxr_wrapper_->RegisterXrEventHandler(eventHandler);
        PLOGI("EXTPerformanceSettings::OnEventHandlerSetup RegisterXrEventHandler succeeded");
        return true;
    }

    int EXTPerformanceSettings::SetPerformanceLevels(XrPerfSettingsDomainEXT which, int level) {
        auto xrSession = openxr_wrapper_->GetXrSession();
        if (xrPerfSettingsSetPerformanceLevelEXT == nullptr) {
            PLOGE("EXTPerformanceSettings::SetPerformanceLevels failed, xrPerfSettingsSetPerformanceLevelEXT is null");
            return -1;
        }
        CHECK_XRCMD(xrPerfSettingsSetPerformanceLevelEXT(xrSession, which, ConvertIntLevelToXr(which, level)));
        PLOGI("EXTPerformanceSettings::SetPerformanceLevels XrPerfSettingsDomainEXT:%d, level:%d", which, level);
        return 0;
    }

    XrPerfSettingsLevelEXT EXTPerformanceSettings::ConvertIntLevelToXr(XrPerfSettingsDomainEXT which, int level) {
        // now cpu and gpu are the same
        switch (level) {
        case 0:  //autofreq for cpu & gpu
            return XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT;
        case 1:  //cpu:1478400  gpu:400000000
            return XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT;
        case 3:  //cpu:2054400  gpu:490000000
            return XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
        case 5:  //cpu:2419200  gpu:587000000
            return XR_PERF_SETTINGS_LEVEL_BOOST_EXT;
            //            case 9999:
            //                return (XrPerfSettingsLevelEXT)100;
        default:
            return XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
        }
    }

    int EXTPerformanceSettings::ConvertXrLevelToInt(XrPerfSettingsDomainEXT which, XrPerfSettingsLevelEXT level) {
        // now cpu and gpu are the same
        switch (level) {
        case XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT:
            return 0;
        case XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT:
            return 1;
        case XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT:
            return 3;
        case XR_PERF_SETTINGS_LEVEL_BOOST_EXT:
            return 5;
            //            case (XrPerfSettingsLevelEXT)100:
            //                return 9999;
        default:
            return 3;
        }
    }

    int EXTPerformanceSettings::InitializePerformanceLevels(int cpuLevel, int gpuLevel) {
        perf_setting_cpu_level_ = cpuLevel;
        perf_setting_gpu_level_ = gpuLevel;
        return XR_SUCCESS;
    }
}  // namespace PVRSampleFW
