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

#ifndef PICONATIVEOPENXRSAMPLES_KHRANDROIDTHREADSETTING_H
#define PICONATIVEOPENXRSAMPLES_KHRANDROIDTHREADSETTING_H

#include <IOpenXRExtensionPlugin.h>
#include "CheckUtils.h"

namespace PVRSampleFW {

    /**
     * doc: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_android_thread_settings
     */
    class KHRAndroidThreadSetting : public IOpenXRExtensionPlugin {
    public:
        KHRAndroidThreadSetting() {
        }
        KHRAndroidThreadSetting(const std::shared_ptr<IXrPlatformPlugin>& plaf, BasicOpenXrWrapper* openxrW)
            : IOpenXRExtensionPlugin(plaf, openxrW) {
        }
        virtual ~KHRAndroidThreadSetting() {
        }

        std::vector<std::string> GetRequiredExtensions() const override;

        bool OnInstanceCreate() override;

        bool OnEventHandlerSetup() override;

        /**
         * Set the android app thread type.
         *
         * @param which thread type
         * @param tid tid of thread
         * @return 0 is success, the others are failed
         */
        int SetAndroidApplicationThreadKHR(XrAndroidThreadTypeKHR which, int tid);

        /**
         * Initialize the tid numbers of main thread and render thread.
         *
         * @param mainThreadTid
         * @param renderThreadTid
         * @return 0 is success, the others are failed
         */
        int InitializeThreadsTid(int mainThreadTid, int renderThreadTid);

    public:
        PFN_DECLARE(xrSetAndroidApplicationThreadKHR);

    private:
        int android_main_thread_tid_{0};
        int android_render_thread_tid_{0};
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_KHRANDROIDTHREADSETTING_H
