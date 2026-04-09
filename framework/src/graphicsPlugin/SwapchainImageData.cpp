// Copyright (c) 2017-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

/*
 * Copyright 2024 - 2025 PICO.
 *
 * This code is a PICO modified version from OpenXR SDK distribution by The Khronos Group Inc.
 *
 * */

#include "SwapchainImageData.h"
#include "CheckUtils.h"

namespace PVRSampleFW {
    ISwapchainImageData::~ISwapchainImageData() = default;

    DepthSwapchainHandling::DepthSwapchainHandling(XrSwapchain depthSwapchain) : depth_swapchain_(depthSwapchain) {
        CHECK(depthSwapchain != XR_NULL_HANDLE);
    }

    void DepthSwapchainHandling::AcquireAndWaitDepthSwapchainImage(uint32_t colorImageIndex) {

        std::unique_lock<std::mutex> lock(mutex_);
        uint32_t depthImageIndex;
        XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        CHECK_XRCMD(xrAcquireSwapchainImage(depth_swapchain_, &acquireInfo, &depthImageIndex));

        XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        waitInfo.timeout = XR_INFINITE_DURATION;  // Call can block waiting for image to become available for writing.
        CHECK_XRCMD(xrWaitSwapchainImage(depth_swapchain_, &waitInfo));
        color_to_acquired_depthIndices_.emplace_back(colorImageIndex, depthImageIndex);
    }

    bool DepthSwapchainHandling::GetWaitedDepthSwapchainImageIndexFor(uint32_t colorImageIndex,
                                                                      uint32_t* outDepthImageIndex) const {
        std::unique_lock<std::mutex> lock(mutex_);
        const auto b = color_to_acquired_depthIndices_.begin();
        const auto e = color_to_acquired_depthIndices_.end();
        auto it = std::find_if(
                b, e, [colorImageIndex](std::pair<uint32_t, uint32_t> const& p) { return p.first == colorImageIndex; });
        if (it == e) {
            return false;
        }
        *outDepthImageIndex = it->second;
        return true;
    }

    void DepthSwapchainHandling::ReleaseDepthSwapchainImage() {

        std::unique_lock<std::mutex> lock(mutex_);
        if (color_to_acquired_depthIndices_.empty()) {
            // over-releasing?
            return;
        }

        XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        CHECK_XRCMD(xrReleaseSwapchainImage(depth_swapchain_, &releaseInfo));
        color_to_acquired_depthIndices_.erase(color_to_acquired_depthIndices_.begin());
    }
}  // namespace PVRSampleFW
