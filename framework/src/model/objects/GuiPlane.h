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

#ifndef PICONATIVEOPENXRSAMPLES_GUIPLANE_H
#define PICONATIVEOPENXRSAMPLES_GUIPLANE_H

#include "Object.h"
#include "IGuiRenderer.h"

namespace PVRSampleFW {
    constexpr float GUI_VERTICES[] = {
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.5f,  0.5f,  0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    };

    constexpr uint32_t GUI_INDICES[] = {
            2, 1, 0, 0, 3, 2,
    };

    class GuiPlane : public Object {
    public:
        GuiPlane(const XrPosef& p, const XrVector3f& s, const std::shared_ptr<GuiWindow>& guiWindow);

        void RayCollisionResult(XrPosef rayOriginPose, XrVector3f point, bool bCollision, bool bTrigger,
                                int side) override;

        uint32_t GetColorTexId() const override;

    private:
        XrVector2f Get2DCoordinatesOnPlane(const XrVector3f& point);

        void BuildObject();

    private:
        std::shared_ptr<GuiWindow> m_guiWindow;
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_GUIPLANE_H
