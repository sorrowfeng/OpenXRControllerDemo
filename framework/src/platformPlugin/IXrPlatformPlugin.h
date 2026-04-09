// Copyright (c) 2017-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

/* This file may have been modified by PICO */

#ifndef PICONATIVEOPENXRSAMPLES_IXRPLATFORMPLUGIN_H
#define PICONATIVEOPENXRSAMPLES_IXRPLATFORMPLUGIN_H

#include "util/Common.h"
#include "pch.h"

namespace PVRSampleFW {
    class IXrPlatformPlugin {
    public:
        virtual ~IXrPlatformPlugin() = default;

        // Provide extension to XrInstanceCreateInfo for xrCreateInstance based on platform.
        // Return nullptr if no extension is needed(win32, posix).
        virtual XrBaseInStructure* GetInstanceCreateInfo() = 0;

        // OpenXR instance-level extensions required by this platform.
        virtual std::vector<std::string> GetInstanceExtensionsRequiredByPlatform() const = 0;

        // Perform required steps after updating Configurations
        virtual void UpdateConfigurationsAtPlatform(const std::shared_ptr<struct Configurations>& config) = 0;
    };
}  // namespace PVRSampleFW
#endif  //PICONATIVEOPENXRSAMPLES_IXRPLATFORMPLUGIN_H
