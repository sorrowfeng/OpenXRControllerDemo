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

#include "GuiPlane.h"
#include "LogUtils.h"
#include "xr_linear.h"
#include "glm/ext/quaternion_float.hpp"
#include "glm/gtc/quaternion.hpp"

namespace PVRSampleFW {
    GuiPlane::GuiPlane(const XrPosef& p, const XrVector3f& s, const std::shared_ptr<GuiWindow>& guiWindow) : Object() {
        type_ = OBJECT_TYPE_GUI_PLANE;
        gl_program_type_ = GL_PROGRAM_TYPE_SAMPLER_2D;
        gl_geometry_type_ = GL_GEOMETRY_TYPE_POS_SAMPLER2D_QUAD;
        this->pose_ = p;
        this->scale_ = s;
        m_guiWindow = guiWindow;
        BuildObject();
        SetClickable(true);
    }

    void GuiPlane::RayCollisionResult(XrPosef rayOriginPose, XrVector3f point, bool bCollision, bool bTrigger,
                                      int side) {
        // handle ancestral class
        Object::RayCollisionResult(rayOriginPose, point, bCollision, bTrigger, side);

        XrVector2f normalPoint{-1, -1};
        if (bCollision) {
            // 计算碰撞点
            normalPoint = Get2DCoordinatesOnPlane(point);
        }

        m_guiWindow->InjectingRayInputEvent(normalPoint.x, normalPoint.y, bCollision, bTrigger, side);
    }

    XrVector2f GuiPlane::Get2DCoordinatesOnPlane(const XrVector3f& point) {
        const XrPosef pose = GetPose();
        glm::vec3 intersectionPoint = glm::vec3(point.x, point.y, point.z);
        glm::mat4 modelMat1 = glm::mat4_cast(
                glm::quat(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z));
        modelMat1[3][0] = pose.position.x;
        modelMat1[3][1] = pose.position.y;
        modelMat1[3][2] = pose.position.z;

        glm::vec4 localPosition = glm::inverse(modelMat1) * glm::vec4(intersectionPoint, 1.0f);

        float u = localPosition.x / scale_.x + 0.5f;
        float v = localPosition.y / scale_.y + 0.5f;

        PLOGD("GuiPlane::Get2DCoordinatesOnPlane position=(%f, %f, %f), (%f, %f)", localPosition.x, localPosition.y,
              localPosition.z, u, v);

        return XrVector2f{u, v};
    }

    uint32_t GuiPlane::GetColorTexId() const {
        return m_guiWindow->GetTextureId();
    }

    void GuiPlane::BuildObject() {
        // vertex buffer
        auto vertexSize = sizeof(GUI_VERTICES);
        vertex_buffer_.resize(vertexSize / sizeof(float));
        std::memcpy(vertex_buffer_.data(), GUI_VERTICES, vertexSize);

        // index buffer
        auto indexSize = sizeof(GUI_INDICES);
        index_buffer_.resize(indexSize / sizeof(uint32_t));
        std::memcpy(index_buffer_.data(), GUI_INDICES, indexSize);

        // Draw mode
        SetDrawMode(DRAW_MODE_TRIANGLES);

        // Set Draw using array
        SetDrawUsingArrays(true);

        // set solid
        SetSolid(true);

        // set movable as default
        SetMovable(true);
    }
}  // namespace PVRSampleFW
