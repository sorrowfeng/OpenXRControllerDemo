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

//
// Created by ByteDance on 12/2/24.
//

#ifndef PICONATIVEOPENXRSAMPLES_FBDISPLAYREFRESHRATES_H
#define PICONATIVEOPENXRSAMPLES_FBDISPLAYREFRESHRATES_H

#include "IOpenXRExtensionPlugin.h"
#include "CheckUtils.h"

namespace PVRSampleFW {

    /**
    * doc：https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_FB_display_refresh_rate
    */
    class FBDisplayRefreshRates : public IOpenXRExtensionPlugin {
    public:
        FBDisplayRefreshRates() = default;

        virtual ~FBDisplayRefreshRates() {
        }

        std::vector<std::string> GetRequiredExtensions() const override;

        bool OnInstanceCreate() override;

        bool OnEventHandlerSetup() override;

        /**
         * Get available refresh rates at this session
         *
         * @note: remember to free rates array memory when not using
         * @param count count of refresh rates
         * @param rateArray refresh rates
         * @return 0 is success, others are failed
         */
        int GetDisplayRefreshRatesAvailable(uint32_t *count, float **rateArray);

        /**
         * Set current refresh rate at this session
         * @param displayRefreshRate
         * @return 0 is success, others are failed
         */
        int RequestDisplayRefreshRateFB(float displayRefreshRate);

        /** Retrieves the current display refresh rate
         *
         * @param displayRefreshRate
         * @return 0 is success, others are failed
         */
        int GetDisplayRefreshRateFB(float *displayRefreshRate);

        /**
         * Set the target refresh rate.
         * @note This action will not take effect immediately, but will take effect when the session is ready.
         *      Use @refitem RequestDisplayRefreshRateFB instead when hope to take effect immediately
         * @param configRefreshRate target refresh rate
         * @return 0 is success, others are failed
         */
        int SetConfiguredRefreshRate(float configRefreshRate);

        /**
         * Get the target refresh rate
         * @param outConfigRefreshRate he target refresh rate
         * @return 0 is success, others are failed
         */
        int GetConfiguredRefreshRate(float *outConfigRefreshRate) const;

        //enumerates the display refresh rates supported by the current session
        int EnumerateDisplayRefreshRates(XrSession session, uint32_t displayRefreshRateCapacityInput,
                                         float *displayRefreshRates);

        int xrEnumerateDisplayRefreshRates(XrSession session, uint32_t displayRefreshRateCapacityInput,
                                           uint32_t *displayRefreshRateCountOutput, float *displayRefreshRates);

        int GetDisplayRefreshRateCount(XrSession session);

    public:
        PFN_DECLARE(xrEnumerateDisplayRefreshRatesFB);
        PFN_DECLARE(xrGetDisplayRefreshRateFB);
        PFN_DECLARE(xrRequestDisplayRefreshRateFB);

    private:
        uint32_t display_refresh_rate_count_output_{0};
        float target_display_refresh_rate_{90.0f};
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_FBDISPLAYREFRESHRATES_H
