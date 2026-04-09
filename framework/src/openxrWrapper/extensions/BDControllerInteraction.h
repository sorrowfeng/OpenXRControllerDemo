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

#ifndef PICONATIVEOPENXRSAMPLES_BDCONTROLLERINTERACTION_H
#define PICONATIVEOPENXRSAMPLES_BDCONTROLLERINTERACTION_H

#include "IOpenXRExtensionPlugin.h"

namespace PVRSampleFW {

    class BDControllerInteraction : public IOpenXRExtensionPlugin {
    public:
        BDControllerInteraction() = default;
        virtual ~BDControllerInteraction() {
        }

        std::vector<std::string> GetRequiredExtensions() const override {
            return {XR_BD_CONTROLLER_INTERACTION_EXTENSION_NAME};
        }
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_BDCONTROLLERINTERACTION_H
