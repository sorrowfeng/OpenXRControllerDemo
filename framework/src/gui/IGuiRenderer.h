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

#ifndef PICONATIVEOPENXRSAMPLES_IGUIRENDERER_H
#define PICONATIVEOPENXRSAMPLES_IGUIRENDERER_H

#include "GuiWindow.h"

namespace PVRSampleFW {

    class IGuiRenderer {
    public:
        virtual ~IGuiRenderer() = default;

        virtual void Initialize(void* context) = 0;

        virtual void Shutdown() = 0;

        virtual void TriggerSignal() = 0;

        virtual uint32_t AddWindow(const std::shared_ptr<GuiWindow>& window) = 0;
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_IGUIRENDERER_H
