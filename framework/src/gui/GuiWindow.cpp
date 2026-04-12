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

#include "GuiWindow.h"
#include "LogUtils.h"
#include "ImGuiRenderer.h"

#include <utility>

namespace PVRSampleFW {
    namespace {
        constexpr int kMinGuiFontSize = 16;
        constexpr int kMaxGuiFontSize = 40;
        constexpr int kGuiFontSizeInterval = 4;

        int NormalizeGuiFontSize(int size) {
            if (size <= kMinGuiFontSize) {
                return kMinGuiFontSize;
            }
            if (size >= kMaxGuiFontSize) {
                return kMaxGuiFontSize;
            }

            const int snapped = kMinGuiFontSize +
                                ((size - kMinGuiFontSize + kGuiFontSizeInterval / 2) / kGuiFontSizeInterval) *
                                        kGuiFontSizeInterval;
            return snapped > kMaxGuiFontSize ? kMaxGuiFontSize : snapped;
        }
    }  // namespace

    GuiWindow::GuiWindow(const WindowConfig &config) : config_(config) {
        //        ImGuiRenderer::GetInstance()->AddWindow(std::shared_ptr<GuiWindow>(this));
        need_render_ = true;
    }

    GuiWindow::~GuiWindow() {
        PLOGI("~GuiWindow");
    }

    void GuiWindow::UpdatePrimaryText(const char *text) {
        if ((config_.flags & GUI_WINDOW_CONFIG_TEXT) != GUI_WINDOW_CONFIG_TEXT) {
            PLOGW("GuiWindow::UpdatePrimaryText is invalid, the window has not set primary text,"
                  "but will add text to it");
        }

        config_.text = text;
        need_render_ = true;
    }

    int GuiWindow::AddButton(const char *name, std::function<void()> callback) {
        WindowComponentConfig config = {
                .flags = 0,
                .componentType = GUI_WINDOW_COMPONENT_TYPE_BUTTON,
                .componentId = component_count_++,
                .name = name,
        };
        config.buttonCallback = callback;

        components_[config.componentId] = config;
        return config.componentId;
    }

    int GuiWindow::AddButton(const char *name, int posX, int posY, std::function<void()> callback) {
        WindowComponentConfig config = {
                .flags = GUI_WINDOW_CONFIG_COMPONENT_POS,
                .componentType = GUI_WINDOW_COMPONENT_TYPE_BUTTON,
                .componentId = component_count_++,
                .posX = posX,
                .posY = posY,
                .name = name,
        };
        config.buttonCallback = callback;

        components_[config.componentId] = config;
        return config.componentId;
    }

    int GuiWindow::AddCheckBox(const char *name, bool *checked) {
        WindowComponentConfig config = {
                .flags = 0,
                .componentType = GUI_WINDOW_COMPONENT_TYPE_CHECKBOX,
                .componentId = component_count_++,
                .name = name,
        };
        config.checked = checked;

        components_[config.componentId] = config;
        return config.componentId;
    }

    int GuiWindow::AddCheckBox(const char *name, int posX, int posY, bool *checked) {
        WindowComponentConfig config = {
                .flags = GUI_WINDOW_CONFIG_COMPONENT_POS,
                .componentType = GUI_WINDOW_COMPONENT_TYPE_CHECKBOX,
                .componentId = component_count_++,
                .posX = posX,
                .posY = posY,
                .name = name,
        };
        config.checked = checked;

        components_[config.componentId] = config;
        return config.componentId;
    }

    int GuiWindow::AddText(const char *text) {
        WindowComponentConfig config = {
                .flags = 0,
                .componentType = GUI_WINDOW_COMPONENT_TYPE_TEXT,
                .componentId = component_count_++,
                .name = text,
        };

        components_[config.componentId] = config;
        return config.componentId;
    }

    int GuiWindow::AddText(const char *text, int posX, int posY) {
        WindowComponentConfig config = {
                .flags = GUI_WINDOW_CONFIG_COMPONENT_POS,
                .componentType = GUI_WINDOW_COMPONENT_TYPE_TEXT,
                .componentId = component_count_++,
                .posX = posX,
                .posY = posY,
                .name = text,
        };

        components_[config.componentId] = config;
        return config.componentId;
    }

    void GuiWindow::UpdateText(int componentId, const char *text) {
        auto component = ValidateAndGetComponent(componentId);
        if (component == nullptr) {
            PLOGE("GuiWindow::UpdateText componentId(%d) is invalid(not found)", componentId);
            return;
        }

        component->name = text;
        need_render_ = true;
    }

    void GuiWindow::SetButtonCallback(int componentId, std::function<void()> callback) {
        auto component = ValidateAndGetComponent(componentId);
        if (component == nullptr) {
            PLOGE("GuiWindow::SetButtonCallback componentId(%d) is invalid(not found)", componentId);
            return;
        }
        if (component->componentType != GUI_WINDOW_COMPONENT_TYPE_BUTTON) {
            PLOGE("GuiWindow::SetButtonCallback componentId(%d) is not a button", componentId);
            return;
        }

        component->buttonCallback = std::move(callback);
        need_render_ = true;
    }

    void GuiWindow::SetComponentPos(int componentId, int posX, int posY) {
        auto component = ValidateAndGetComponent(componentId);
        if (component == nullptr) {
            PLOGE("GuiWindow::SetComponentPos componentId(%d) is invalid(not found)", componentId);
            return;
        }

        component->posX = posX;
        component->posY = posY;
        component->flags |= GUI_WINDOW_CONFIG_COMPONENT_POS;
        need_render_ = true;
    }

    void GuiWindow::SetComponentSize(int componentId, int width, int height) {
        auto component = ValidateAndGetComponent(componentId);
        if (component == nullptr) {
            PLOGE("GuiWindow::SetComponentSize componentId(%d) is invalid(not found)", componentId);
            return;
        }

        component->width = width;
        component->height = height;
        component->flags |= GUI_WINDOW_CONFIG_COMPONENT_SIZE;
        need_render_ = true;
    }

    void GuiWindow::SetComponentBgColor(int componentId, float r, float g, float b, float a) {
        auto component = ValidateAndGetComponent(componentId);
        if (component == nullptr) {
            PLOGE("GuiWindow::SetComponentBgColor componentId(%d) is invalid(not found)", componentId);
            return;
        }

        component->bgColor[0] = r;
        component->bgColor[1] = g;
        component->bgColor[2] = b;
        component->bgColor[3] = a;
        component->flags |= GUI_WINDOW_CONFIG_COMPONENT_BG_COLOR;
        need_render_ = true;
    }

    void GuiWindow::SetComponentTextColor(int componentId, float r, float g, float b, float a) {
        auto component = ValidateAndGetComponent(componentId);
        if (component == nullptr) {
            PLOGE("GuiWindow::SetComponentTextColor componentId(%d) is invalid(not found)", componentId);
            return;
        }

        component->textColor[0] = r;
        component->textColor[1] = g;
        component->textColor[2] = b;
        component->textColor[3] = a;
        component->flags |= GUI_WINDOW_CONFIG_TEXT_COLOR;
        need_render_ = true;
    }

    void GuiWindow::SetComponentTextSize(int componentId, int size) {
        auto component = ValidateAndGetComponent(componentId);
        if (component == nullptr) {
            PLOGE("GuiWindow::SetComponentTextColor componentId(%d) is invalid(not found)", componentId);
            return;
        }

        const int normalizedSize = NormalizeGuiFontSize(size);
        if (normalizedSize != size) {
            PLOGW("GuiWindow::SetComponentTextSize normalize size from %d to %d", size, normalizedSize);
        }
        component->textSize = normalizedSize;
        component->flags |= GUI_WINDOW_CONFIG_COMPONENT_FONT_SIZE;
        need_render_ = true;
    }

    void GuiWindow::InjectingRayInputEvent(float x, float y, bool bCollision, bool bTrigger, int side) {
        if (side < 0 || side > 1) {
            PLOGE("GuiWindow::InjectingRayInputEvent side(%d) is invalid", side);
            return;
        }

        if (!bCollision) {
            collision_state_[side].pointPixelX = -10.0f;
            collision_state_[side].pointPixelY = -10.0f;
            collision_state_[side].bLastCollision = collision_state_[side].bCollision;
            collision_state_[side].bLastTrigger = collision_state_[side].bTrigger;
            collision_state_[side].bCollision = bCollision;
            collision_state_[side].bTrigger = bTrigger;
            return;
        }

        if (side == 0) {
            need_render_ = collision_state_[side].bCollision != collision_state_[side].bLastCollision ||
                           collision_state_[side].bCollision;
        } else {
            need_render_ |= collision_state_[side].bCollision != collision_state_[side].bLastCollision ||
                            collision_state_[side].bCollision;
        }

        collision_state_[side].pointPixelX = x * config_.width;
        collision_state_[side].pointPixelY = y * config_.height;
        collision_state_[side].bLastCollision = collision_state_[side].bCollision;
        collision_state_[side].bLastTrigger = collision_state_[side].bTrigger;
        collision_state_[side].bCollision = bCollision;
        collision_state_[side].bTrigger = bTrigger;
    }

    WindowComponentConfig *GuiWindow::ValidateAndGetComponent(int componentId) {
        if (componentId < 0 || componentId >= component_count_) {
            PLOGE("GuiWindow::ValidateAndGetComponent componentId(%d) is invalid(out of range)", componentId);
            return nullptr;
        }
        auto component = components_.find(componentId);
        if (component == components_.end()) {
            PLOGE("GuiWindow::ValidateAndGetComponent componentId(%d) is invalid(not found)", componentId);
            return nullptr;
        }

        return &component->second;
    }

    RayInputEvent GuiWindow::GetCurrentRayInputEvent() {
        // check collision state
        for (int i = 0; i < 2; i++) {
            ray_input_.coord_pixel[i].first = collision_state_[i].pointPixelX;
            ray_input_.coord_pixel[i].second = collision_state_[i].pointPixelY;
            ray_input_.collision[i] = collision_state_[i].bCollision;
        }

        // record last state
        ray_input_.last_effective_side = ray_input_.effective_side;

        // calculate effective side
        // collision state same, check last collision state
        if ((collision_state_[0].bCollision && collision_state_[1].bCollision) ||
            (!collision_state_[0].bCollision && !collision_state_[1].bCollision)) {
            // check last trigger state
            if ((collision_state_[0].bTrigger && collision_state_[1].bTrigger) ||
                (!collision_state_[0].bTrigger && !collision_state_[1].bTrigger)) {
                // last trigger state same, keep last effective side
                ray_input_.effective_side = ray_input_.last_effective_side;
            } else {
                // last trigger state different, return last trigger side
                ray_input_.effective_side = (collision_state_[1].bTrigger) ? 1 : 0;
            }
        } else {
            // collision state different, return collision side
            ray_input_.effective_side = collision_state_[1].bCollision ? 1 : 0;
        }

        // check trigger state
        ray_input_.triggered = collision_state_[ray_input_.effective_side].bTrigger;

        return ray_input_;
    }

    std::shared_ptr<GuiWindow> GuiWindow::GenerateButtonWindow(const std::vector<ButtonPair> &buttons) {
        int buttonCount = buttons.size();
        if (buttonCount < 1 || buttonCount > 8) {
            PLOGE("The number of buttons is out of range(1~8)");
        }

        const int col = (buttonCount - 1) / BUTTON_ROW_MAX + 1;
        const int row = col > 1 ? BUTTON_ROW_MAX : buttonCount % (BUTTON_ROW_MAX + 1);
        int width = col * (BUTTON_WIDTH + BUTTON_INTERVAL_X);
        int height = row * (BUTTON_HEIGHT + BUTTON_INTERVAL_Y);

        Builder builder;
        std::shared_ptr<GuiWindow> window = builder.SetSize(width, height).NoScrollbar().Build();
        for (int i = 0; i < buttonCount; ++i) {
            auto &button = buttons[i];
            const int xOffset = (i / BUTTON_ROW_MAX) * (BUTTON_WIDTH + BUTTON_INTERVAL_X) + BUTTON_INTERVAL_X / 2;
            const int yOffset = (i % BUTTON_ROW_MAX) * (BUTTON_HEIGHT + BUTTON_INTERVAL_Y) + BUTTON_INTERVAL_Y / 2;
            int componentId = window->AddButton(button.name, xOffset, yOffset, button.callback);
            window->SetComponentSize(componentId, BUTTON_WIDTH, BUTTON_HEIGHT);
        }

        return window;
    }

    std::shared_ptr<GuiWindow> GuiWindow::Builder::Build() {
        auto window = std::make_shared<GuiWindow>(config_);
        ImGuiRenderer::GetInstance()->AddWindow(window);
        return window;
    }
}  // namespace PVRSampleFW
