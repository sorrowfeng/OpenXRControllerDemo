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

#include "Object.h"
#include "LogUtils.h"
#include "GuiPlane.h"

namespace PVRSampleFW {
    int DrawModeToGlPrimitive(DrawMode mode) {
        switch (mode) {
        case DRAW_MODE_POINTS:
            return GL_POINTS;
        case DRAW_MODE_LINES:
            return GL_LINES;
        case DRAW_MODE_LINE_STRIP:
            return GL_LINE_STRIP;
        case DRAW_MODE_LINE_LOOP:
            return GL_LINE_LOOP;
        case DRAW_MODE_TRIANGLES:
            return GL_TRIANGLES;
        case DRAW_MODE_TRIANGLE_STRIP:
            return GL_TRIANGLE_STRIP;
        case DRAW_MODE_TRIANGLE_FAN:
            return GL_TRIANGLE_FAN;
        default:
            return -1;
        }
    }

    void Object::RayCollisionResult(XrPosef rayOriginPose, XrVector3f point, bool bCollision, bool bTrigger, int side) {
        // filter invalid side
        if (side < 0 || side > 1) {
            return;
        }

        constexpr int kLongPressThreshold = 10;

        if (IsSolid()) {
            input_status_.last_hover_[side] = input_status_.current_hover_[side];
            input_status_.current_hover_[side] = bCollision && !bTrigger;
            input_status_.last_press_[side] = input_status_.last_press_[side];
            input_status_.current_press_[side] = bCollision && bTrigger;

            if (bCollision && !bTrigger) {
                input_status_.hover_in_cnt_[side]++;
            } else {
                input_status_.hover_in_cnt_[side] = 0;
            }

            if (bCollision && bTrigger) {
                input_status_.press_cnt_[side]++;
            } else {
                input_status_.press_cnt_[side] = 0;
                // reset relative pose
                input_status_.pose_to_ray_origin_[side] = {
                        {0.0f, 0.0f, 0.0f, 1.0f},
                        {0.0f, 0.0f, 0.0f},
                };
            }

            if (input_status_.press_cnt_[0] == 0 && input_status_.press_cnt_[1] == 0) {
                // reset bound to ray origin
                input_status_.bound_to_ray_ = -1;
            }

            input_status_.last_collide_point_[side] = input_status_.current_collide_point_[side];
            if (bCollision) {
                input_status_.current_collide_point_[side] = point;
            } else {
                // invalid point
                input_status_.current_collide_point_[side] = {FLT_MAX, FLT_MAX, FLT_MAX};
            }

            // record relative pose to ray origin when long press first occur
            if (input_status_.press_cnt_[side] == kLongPressThreshold && input_status_.bound_to_ray_ == -1) {
                input_status_.bound_to_ray_ = side;
                // calculate relative pose to ray origin
                XrPosef inverseRayOriginPose;
                XrPosef_Invert(&inverseRayOriginPose, &rayOriginPose);
                XrPosef_Multiply(&input_status_.pose_to_ray_origin_[side], &inverseRayOriginPose, &pose_);
            }

            // hover callback
            if (nullptr != hover_callback_) {
                hover_callback_(input_status_.current_hover_[side]);
            }

            // click callback
            auto clicked = !input_status_.current_press_[side] && input_status_.last_press_[side] &&
                           input_status_.press_cnt_[side] <= kLongPressThreshold;
            if (nullptr != click_callback_ && IsClickable()) {
                click_callback_(clicked);
            }

            // handle move action
            if (IsMovable()) {
                // check if bound to parent, only object can be moved when not bound to parent
                if (!is_parent_bound_ || nullptr == parent_) {
                    if (input_status_.press_cnt_[side] > kLongPressThreshold && input_status_.bound_to_ray_ == side) {
                        auto relativePose = input_status_.pose_to_ray_origin_[side];
                        XrPosef_Multiply(&pose_, &rayOriginPose, &relativePose);
                    }
                }
            }
        }
    }

    void Object::AddTextLabel(const char *text) {
        GuiWindow::Builder builder;
        std::shared_ptr<GuiWindow> window =
                builder.SetSize(200, 50).SetText(text).SetFontSize(24).SetTextColor(0.1f, 0.8f, 0.1f, 1.0f).Build();
        AddWindowLabel(window, false);
    }

    void Object::AddTextLabel(const char *text, const XrPosef &pose, const XrVector2f &extent,
                              const XrVector2f &windowPixelSize) {
        GuiWindow::Builder builder;
        std::shared_ptr<GuiWindow> window = builder.SetSize(windowPixelSize.x, windowPixelSize.y)
                                                    .SetText(text)
                                                    .SetFontSize(24)
                                                    .SetTextColor(0.1f, 0.8f, 0.1f, 1.0f)
                                                    .Build();
        AddWindowLabel(window, pose, extent, false);
    }

    int Object::AddButtonLabel(const std::vector<ButtonPair> &buttons) {
        auto window = GuiWindow::GenerateButtonWindow(buttons);
        if (window == nullptr) {
            PLOGE("AddButtonLabel error");
            return -1;
        }

        AddWindowLabel(window, true);
        return 0;
    }

    void Object::AddWindowLabel(const std::shared_ptr<GuiWindow> &window, bool solid) {
        constexpr float meterPerPixel = 0.003f;
        auto config = window->GetConfig();
        XrVector2f extent = {config.width * meterPerPixel, config.height * meterPerPixel};
        XrPosef guiPose = {{0.0f, 0.0f, 0.0f, 1.0f},
                           {(scale_.x + extent.x) / 2.0f, (scale_.y + extent.y) / 2.0f, scale_.z / 2.0f + 0.01f}};
        AddWindowLabel(window, guiPose, extent, solid);
    }

    void Object::AddWindowLabel(const std::shared_ptr<GuiWindow> &window, const XrPosef &pose, const XrVector2f &extent,
                                bool solid) {
        auto guiPlane = std::make_shared<GuiPlane>(pose, XrVector3f{extent.x, extent.y, 1.0f}, window);
        guiPlane->SetRenderDepthable(false);
        guiPlane->SetSolid(solid);
        AddChild(guiPlane);
    }

    void Object::AddChild(const std::shared_ptr<Object> &child) {
        children_.insert(child);
        child->SetParent(this);
    }

    void Object::SetParent(Object *parent) {
        parent_ = parent;
        is_parent_bound_ = true;
    }

    XrPosef Object::GetPose() {
        if (is_parent_bound_ && nullptr != parent_) {
            XrPosef parentPose = parent_->GetPose();
            XrPosef pose;
            XrPosef_Multiply(&pose, &parentPose, &pose_);
            return pose;
        }

        return pose_;
    }

    void Object::RegisterHoverCallback(std::function<void(bool)> callback) {
        SetSolid(true);
        hover_callback_ = callback;
    }

    void Object::RegisterClickCallback(std::function<void(bool)> callback) {
        SetSolid(true);
        SetClickable(true);
        click_callback_ = callback;
    }
}  // namespace PVRSampleFW
