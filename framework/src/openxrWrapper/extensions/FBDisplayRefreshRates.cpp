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

#include "FBDisplayRefreshRates.h"
#include "BasicOpenXrWrapper.h"
#include <algorithm>

namespace PVRSampleFW {
    std::vector<std::string> FBDisplayRefreshRates::GetRequiredExtensions() const {
        return {XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME};
    }

    bool FBDisplayRefreshRates::OnInstanceCreate() {
        // Initialize the function pointers
        if (openxr_wrapper_ == nullptr) {
            PLOGE("EXTPerformanceSettings::OnInstanceCreate failed for openxr_wrapper_ is null");
            return false;
        }

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrEnumerateDisplayRefreshRatesFB",
                                          reinterpret_cast<PFN_xrVoidFunction *>(&xrEnumerateDisplayRefreshRatesFB)));

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrGetDisplayRefreshRateFB",
                                          reinterpret_cast<PFN_xrVoidFunction *>(&xrGetDisplayRefreshRateFB)));

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrRequestDisplayRefreshRateFB",
                                          reinterpret_cast<PFN_xrVoidFunction *>(&xrRequestDisplayRefreshRateFB)));

        return true;
    }

    bool FBDisplayRefreshRates::OnEventHandlerSetup() {
        /// Here is a demonstration of handling a generic xrEvent.
        if (openxr_wrapper_ == nullptr) {
            PLOGE("EXTPerformanceSettings::OnEventHandlerSetup failed for openxr_wrapper_ is null");
            return false;
        }
        // Register the event handler
        XrEventHandler eventHandler = {
                .event_type = XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB,
                .handler =
                        [this](BasicOpenXrWrapper *userData, const XrEventDataBaseHeader *eventData,
                               bool *exitRenderLoop, bool *requestRestart) {
                            float display_refresh_rate = 0;
                            this->GetDisplayRefreshRateFB(&display_refresh_rate);
                            PLOGI("FBDisplayRefreshRates current refresh rate changed to %f", display_refresh_rate);
                            /// TODO: Handle the event.
                            BasicOpenXrWrapper *openxr = userData;
                            const XrEventDataDisplayRefreshRateChangedFB *const displayRefreshRateChangedFB =
                                    reinterpret_cast<const XrEventDataDisplayRefreshRateChangedFB *>(eventData);
                            float from_display_refresh_rate = displayRefreshRateChangedFB->fromDisplayRefreshRate;
                            float to_display_refresh_rate = displayRefreshRateChangedFB->toDisplayRefreshRate;
                            PLOGI("FBDisplayRefreshRates current refresh rate changed from %f to %f",
                                  from_display_refresh_rate, to_display_refresh_rate);
                        },
        };

        openxr_wrapper_->RegisterXrEventHandler(eventHandler);

        // Register the event handler
        XrEventHandler eventHandler2 = {
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
                                    PLOGI("FBDisplayRefreshRates::OnEventHandlerSetup XR_SESSION_STATE_READY");
                                    // Set the performance level
                                    uint32_t dispaly_player_count = 0;
                                    float *display_refresh_rates = nullptr;
                                    this->GetDisplayRefreshRatesAvailable(&dispaly_player_count,
                                                                          &display_refresh_rates);
                                    float max_display_refresh_rate = 0;
                                    bool is_target_display_refresh_rate_available = false;
                                    if (dispaly_player_count > 0 && display_refresh_rates != nullptr) {
                                        for (uint32_t i = 0; i < dispaly_player_count; i++) {
                                            PLOGI("FBDisplayRefreshRates available display_refresh_rates[%d]: %f", i,
                                                  display_refresh_rates[i]);
                                            max_display_refresh_rate =
                                                    std::max(max_display_refresh_rate, display_refresh_rates[i]);
                                            if (display_refresh_rates[i] == target_display_refresh_rate_) {
                                                is_target_display_refresh_rate_available = true;
                                            }
                                        }
                                        /// If target_display_refresh_rate_ is not available,
                                        /// set to max_display_refresh_rate instead.
                                        if (!is_target_display_refresh_rate_available) {
                                            PLOGW("FBDisplayRefreshRates target_display_refresh_rate_: %f "
                                                  "is not available, set to max_display_refresh_rate: %f",
                                                  target_display_refresh_rate_, max_display_refresh_rate);
                                            target_display_refresh_rate_ = max_display_refresh_rate;
                                        }
                                        this->RequestDisplayRefreshRateFB(target_display_refresh_rate_);
                                        if (dispaly_player_count > 0 && display_refresh_rates != nullptr) {
                                            delete[] display_refresh_rates;
                                        }
                                    }
                                } else {
                                    PLOGW("FBDisplayRefreshRates::OnEventHandlerSetup set failed, session not running");
                                }
                            }
                        },
        };

        openxr_wrapper_->RegisterXrEventHandler(eventHandler2);
        PLOGI("EXTPerformanceSettings::OnEventHandlerSetup RegisterXrEventHandler succeeded");
        return true;
    }

    int FBDisplayRefreshRates::EnumerateDisplayRefreshRates(XrSession session, uint32_t displayRefreshRateCapacityInput,
                                                            float *displayRefreshRates) {
        CHECK_POINTER_ARG_IS_NOT_NULL(xrEnumerateDisplayRefreshRatesFB);
        return xrEnumerateDisplayRefreshRatesFB(session, displayRefreshRateCapacityInput,
                                                &display_refresh_rate_count_output_, displayRefreshRates);
    }

    int FBDisplayRefreshRates::xrEnumerateDisplayRefreshRates(XrSession session,
                                                              uint32_t displayRefreshRateCapacityInput,
                                                              uint32_t *displayRefreshRateCountOutput,
                                                              float *displayRefreshRates) {
        CHECK_POINTER_ARG_IS_NOT_NULL(xrEnumerateDisplayRefreshRatesFB);
        return xrEnumerateDisplayRefreshRatesFB(session, displayRefreshRateCapacityInput, displayRefreshRateCountOutput,
                                                displayRefreshRates);
    }

    int FBDisplayRefreshRates::GetDisplayRefreshRateCount(XrSession session) {
        CHECK_POINTER_ARG_IS_NOT_NULL(xrEnumerateDisplayRefreshRatesFB);
        if (session == XR_NULL_HANDLE) {
            PLOGE("FBDisplayRefreshRates::GetDisplayRefreshRateCount session is null");
            return -1;
        }
        if (display_refresh_rate_count_output_ == 0) {
            xrEnumerateDisplayRefreshRatesFB(session, 0, &display_refresh_rate_count_output_, nullptr);
        }
        return display_refresh_rate_count_output_;
    }

    int FBDisplayRefreshRates::GetDisplayRefreshRateFB(float *displayRefreshRate) {
        CHECK_POINTER_ARG_IS_NOT_NULL(xrGetDisplayRefreshRateFB);
        int ret = xrGetDisplayRefreshRateFB(openxr_wrapper_->GetXrSession(), displayRefreshRate);
        PLOGI("FBDisplayRefreshRates::GetDisplayRefreshRateFB ret: %d, rate: %f", ret, *displayRefreshRate);
        return ret;
    }

    int FBDisplayRefreshRates::RequestDisplayRefreshRateFB(float displayRefreshRate) {
        CHECK_POINTER_ARG_IS_NOT_NULL(xrRequestDisplayRefreshRateFB);
        int ret = xrRequestDisplayRefreshRateFB(openxr_wrapper_->GetXrSession(), displayRefreshRate);
        PLOGI("FBDisplayRefreshRates::RequestDisplayRefreshRateFB ret: %d, rate: %f", ret, displayRefreshRate);
        return ret;
    }

    int FBDisplayRefreshRates::GetDisplayRefreshRatesAvailable(uint32_t *count, float **rateArray) {
        int display_frequency_count = GetDisplayRefreshRateCount(openxr_wrapper_->GetXrSession());
        if (display_frequency_count <= 0) {
            *rateArray = nullptr;
            return -1;
        }
        float *display_frequency_array = new float[display_frequency_count];
        int ret = EnumerateDisplayRefreshRates(openxr_wrapper_->GetXrSession(), display_frequency_count,
                                               display_frequency_array);
        if (ret != XR_SUCCESS) {
            delete[] display_frequency_array;
            *rateArray = nullptr;
            return -1;
        }
        *count = display_frequency_count;
        *rateArray = display_frequency_array;
        return 0;
    }

    int FBDisplayRefreshRates::SetConfiguredRefreshRate(float configRefreshRate) {
        target_display_refresh_rate_ = configRefreshRate;
        return 0;
    }

    int FBDisplayRefreshRates::GetConfiguredRefreshRate(float *outConfigRefreshRate) const {
        *outConfigRefreshRate = target_display_refresh_rate_;
        return 0;
    }
}  // namespace PVRSampleFW
