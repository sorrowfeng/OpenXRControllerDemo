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

#ifndef PICONATIVEOPENXRSAMPLES_EXTFUTURE_H
#define PICONATIVEOPENXRSAMPLES_EXTFUTURE_H

#include "IOpenXRExtensionPlugin.h"
#include "CheckUtils.h"

namespace PVRSampleFW {

    /**
     * doc: https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html#XR_EXT_future
     */
    class EXTFuture : public IOpenXRExtensionPlugin {
    public:
        EXTFuture() = default;
        EXTFuture(const std::shared_ptr<IXrPlatformPlugin>& plaf, BasicOpenXrWrapper* openxrW)
            : IOpenXRExtensionPlugin(plaf, openxrW) {
        }
        virtual ~EXTFuture() = default;

        std::vector<std::string> GetRequiredExtensions() const override;

        bool OnInstanceCreate() override;

        /**
         * Poll future which can check the future status.
         *
         * @param pollInfo poll info of the future
         * @param pollResult status of the future
         * @return 0 is success, the others are failed
         */
        int PollFutureEXT(const XrFuturePollInfoEXT* pollInfo, XrFuturePollResultEXT* pollResult);

        /**
         * Cancel the future.
         *
         * @param cancelInfo info of cancel request
         * @return 0 is success, the others are failed
         */
        int CancelFutureEXT(const XrFutureCancelInfoEXT* cancelInfo);

    public:
        PFN_DECLARE(xrPollFutureEXT);
        PFN_DECLARE(xrCancelFutureEXT);
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_EXTFUTURE_H
