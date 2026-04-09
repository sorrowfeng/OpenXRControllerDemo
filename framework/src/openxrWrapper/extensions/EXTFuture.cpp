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

#include "EXTFuture.h"
#include "BasicOpenXrWrapper.h"

namespace PVRSampleFW {
    std::vector<std::string> EXTFuture::GetRequiredExtensions() const {
        return {XR_EXT_FUTURE_EXTENSION_NAME};
    }

    bool EXTFuture::OnInstanceCreate() {
        // Initialize the function pointers
        if (openxr_wrapper_ == nullptr) {
            PLOGE("EXTPerformanceSettings::OnInstanceCreate failed for openxr_wrapper_ is null");
            return false;
        }

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrPollFutureEXT",
                                          reinterpret_cast<PFN_xrVoidFunction *>(&xrPollFutureEXT)));

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrCancelFutureEXT",
                                          reinterpret_cast<PFN_xrVoidFunction *>(&xrCancelFutureEXT)));
        return true;
    }

    int EXTFuture::PollFutureEXT(const XrFuturePollInfoEXT *pollInfo, XrFuturePollResultEXT *pollResult) {
        if (nullptr == openxr_wrapper_ || nullptr == pollInfo || nullptr == pollResult || nullptr == xrPollFutureEXT) {
            PLOGE("EXTFuture::PollFutureEXT failed for openxr_wrapper_ is null");
            return -1;
        }
        auto xrInstance = openxr_wrapper_->GetXrInstance();
        if (xrInstance == XR_NULL_HANDLE) {
            PLOGE("EXTFuture::PollFutureEXT failed for xr_instance_ is null");
            return -1;
        }

        CHECK_XRCMD(xrPollFutureEXT(xrInstance, pollInfo, pollResult));

        return 0;
    }

    int EXTFuture::CancelFutureEXT(const XrFutureCancelInfoEXT *cancelInfo) {
        if (nullptr == openxr_wrapper_ || nullptr == cancelInfo || nullptr == xrCancelFutureEXT) {
            PLOGE("EXTFuture::CancelFutureEXT failed for openxr_wrapper_ is null");
            return -1;
        }
        auto xrInstance = openxr_wrapper_->GetXrInstance();
        if (xrInstance == XR_NULL_HANDLE) {
            PLOGE("EXTFuture::CancelFutureEXT failed for xr_instance_ is null");
            return -1;
        }

        CHECK_XRCMD(xrCancelFutureEXT(xrInstance, cancelInfo));

        return 0;
    }
}  // namespace PVRSampleFW
