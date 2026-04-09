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

#include "KHRAndroidThreadSetting.h"
#include "BasicOpenXrWrapper.h"

namespace PVRSampleFW {
    std::vector<std::string> KHRAndroidThreadSetting::GetRequiredExtensions() const {
        return {XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME};
    }

    bool KHRAndroidThreadSetting::OnInstanceCreate() {
        // Initialize the function pointers
        if (openxr_wrapper_ == nullptr) {
            PLOGE("KHRAndroidThreadSetting::OnInstanceCreate failed for openxr_wrapper_ is null");
            return false;
        }

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrSetAndroidApplicationThreadKHR",
                                          reinterpret_cast<PFN_xrVoidFunction *>(&xrSetAndroidApplicationThreadKHR)));
        return true;
    }

    bool KHRAndroidThreadSetting::OnEventHandlerSetup() {
        /// Here is a demonstration of handling a generic xrEvent.
        if (openxr_wrapper_ == nullptr) {
            PLOGE("KHRAndroidThreadSetting::OnEventHandlerSetup failed for openxr_wrapper_ is null");
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
                                    PLOGI("KHRAndroidThreadSetting::OnEventHandlerSetup XR_SESSION_STATE_READY");
                                    // Set the performance level
                                    this->SetAndroidApplicationThreadKHR(XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR,
                                                                         android_main_thread_tid_);
                                    this->SetAndroidApplicationThreadKHR(XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR,
                                                                         android_render_thread_tid_);
                                } else {
                                    PLOGW("KHRAndroidThreadSetting::OnEventHandlerSetup set failed,"
                                          " session not running");
                                }
                            }
                        },
        };

        openxr_wrapper_->RegisterXrEventHandler(eventHandler);
        PLOGI("KHRAndroidThreadSetting::OnEventHandlerSetup RegisterXrEventHandler succeeded");
        return true;
    }

    int KHRAndroidThreadSetting::SetAndroidApplicationThreadKHR(XrAndroidThreadTypeKHR which, int tid) {
        auto xrSession = openxr_wrapper_->GetXrSession();
        if (xrSetAndroidApplicationThreadKHR == nullptr) {
            PLOGE("EXTPerformanceSettings::SetPerformanceLevels failed, xrPerfSettingsSetPerformanceLevelEXT is null");
            return -1;
        }
        CHECK_XRCMD(xrSetAndroidApplicationThreadKHR(xrSession, which, tid));
        PLOGI("KHRAndroidThreadSetting::SetAndroidApplicationThreadKHR XrAndroidThreadTypeKHR: %d, tid: %d", which,
              tid);
        return 0;
    }

    int KHRAndroidThreadSetting::InitializeThreadsTid(int mainThreadTid, int renderThreadTid) {
        android_main_thread_tid_ = mainThreadTid;
        android_render_thread_tid_ = renderThreadTid;
        return XR_SUCCESS;
    }
}  // namespace PVRSampleFW
