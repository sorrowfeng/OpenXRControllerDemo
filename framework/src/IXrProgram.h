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

#ifndef PICONATIVEOPENXRSAMPLES_IXRPROGRAM_H
#define PICONATIVEOPENXRSAMPLES_IXRPROGRAM_H

#include <vector>

namespace PVRSampleFW {
    class IXrProgram {
    public:
        virtual ~IXrProgram() = default;

        virtual void Initialize() = 0;

        virtual void Shutdown() = 0;

        // Process any pending events and update the application state.
        virtual int PollEvents() = 0;

        virtual std::string GetApplicationName() = 0;

        virtual std::vector<uint8_t> LoadFileFromAsset(const std::string& filename) const = 0;
    };
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_IXRPROGRAM_H
