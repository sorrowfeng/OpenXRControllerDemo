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

#ifndef PICONATIVEOPENXRSAMPLES_GUIWINDOW_H
#define PICONATIVEOPENXRSAMPLES_GUIWINDOW_H

#include <functional>
#include <unordered_map>
#include <string>
#include <vector>

namespace PVRSampleFW {

    enum GuiWindowConfigFlags {
        GUI_WINDOW_CONFIG_WINDOW_SIZE = 1 << 0,
        GUI_WINDOW_CONFIG_BG_COLOR = 1 << 1,
        GUI_WINDOW_CONFIG_TITLE = 1 << 2,
        GUI_WINDOW_CONFIG_TEXT = 1 << 3,
        GUI_WINDOW_CONFIG_TEXT_COLOR = 1 << 4,
        GUI_WINDOW_CONFIG_FONT_SIZE = 1 << 5,
        GUI_WINDOW_CONFIG_COMPONENT_POS = 1 << 6,
        GUI_WINDOW_CONFIG_COMPONENT_SIZE = 1 << 7,
        GUI_WINDOW_CONFIG_COMPONENT_BG_COLOR = 1 << 8,
        GUI_WINDOW_CONFIG_COMPONENT_FONT_SIZE = 1 << 9,
        GUI_WINDOW_CONFIG_NO_SCROLLBAR = 1 << 10,
    };

    enum GuiWindowComponentType {
        GUI_WINDOW_COMPONENT_TYPE_TEXT = 0,
        GUI_WINDOW_COMPONENT_TYPE_BUTTON = 1,
        GUI_WINDOW_COMPONENT_TYPE_CHECKBOX = 2,
    };

    struct ButtonPair {
        const char* name;
        std::function<void()> callback;
    };

    struct CollisionState {
        bool bCollision{false};
        bool bLastCollision{false};  // record the collision status at last frame
        bool bTrigger{false};        // record the trigger status
        bool bLastTrigger{false};
        float pointPixelX{-10.0f};
        float pointPixelY{-10.0f};
    };

    struct RayInputEvent {
        std::pair<float, float> coord_pixel[2]{{-10.0f, -10.0f}, {-10.0f, -10.0f}};
        bool collision[2]{false, false};
        int effective_side{1};       // 0 is left, 1 is right (default)
        int last_effective_side{1};  // 0 is left, 1 is right (default)
        bool triggered{false};
    };

    struct WindowConfig {
        uint32_t flags{0};
        std::string title;
        std::string text;
        int width{400};
        int height{300};
        float bgColor[4]{0.0f, 0.0f, 0.0f, 1.0f};
        float textColor[4]{1.0f, 1.0f, 1.0f, 1.0f};
        int textSize{24};
    };

    struct WindowComponentConfig {
        uint32_t flags{0};
        GuiWindowComponentType componentType;
        int componentId;
        int posX{0};
        int posY{0};
        int width{0};
        int height{0};
        float bgColor[4]{0.0f, 0.0f, 0.0f, 1.0f};
        float textColor[4]{1.0f, 1.0f, 1.0f, 1.0f};
        int textSize{24};
        std::string name;
        bool* checked;
        bool lastCheck{false};
        bool currentCheck{false};
        std::function<void()> buttonCallback;
        std::function<void(bool)> checkBoxCallback;
    };

    using GuiWindowCustomRenderCallback = std::function<void()>;

    class GuiWindow {
    public:
        class Builder {
        public:
            Builder() : config_{GUI_WINDOW_CONFIG_WINDOW_SIZE, "Default Window", "Hello, World!", 400, 300} {
            }

            /**
             * Set the title column information
             * @param title The name that will be displayed on the Title.
             * @return itself
             */
            Builder& SetTitle(const char* title) {
                config_.flags |= GUI_WINDOW_CONFIG_TITLE;
                config_.title = title;
                return *this;
            }

            /**
             * Set the width and height of the window_ display in pixels
             * @param width
             * @param height
             * @return itself
             */
            Builder& SetSize(int width, int height) {
                config_.flags |= GUI_WINDOW_CONFIG_WINDOW_SIZE;
                config_.width = width;
                config_.height = height;
                return *this;
            }

            /**
             * Set the window_ description
             * @param text A string describing the information.
             * @return itself
             */
            Builder& SetText(const char* text) {
                config_.flags |= GUI_WINDOW_CONFIG_TEXT;
                config_.text = text;
                return *this;
            }

            /**
             * Set the background color
             * @param r The red component of the background.
             * @param g The green component of the background.
             * @param b The blue component of the background.
             * @param a The alpha component of the background.
             * @return itself
             */
            Builder& SetBgColor(float r, float g, float b, float a) {
                config_.flags |= GUI_WINDOW_CONFIG_BG_COLOR;
                config_.bgColor[0] = r;
                config_.bgColor[1] = g;
                config_.bgColor[2] = b;
                config_.bgColor[3] = a;
                return *this;
            }

            /**
             * Set the global font size
             * @param size The value range is (16, 40) and the interval is 4
             * @return itself
             */
            Builder& SetFontSize(int size) {
                config_.flags |= GUI_WINDOW_CONFIG_FONT_SIZE;
                config_.textSize = size;
                return *this;
            }

            /**
             * Set the global text color
             * @param r The red component of the text.
             * @param g The green component of the text.
             * @param b The blue component of the text.
             * @param a The alpha component of the text.
             * @return itself
             */
            Builder& SetTextColor(float r, float g, float b, float a) {
                config_.flags |= GUI_WINDOW_CONFIG_TEXT_COLOR;
                config_.textColor[0] = r;
                config_.textColor[1] = g;
                config_.textColor[2] = b;
                config_.textColor[3] = a;
                return *this;
            }

            Builder& NoScrollbar() {
                config_.flags |= GUI_WINDOW_CONFIG_NO_SCROLLBAR;
                return *this;
            }

            std::shared_ptr<GuiWindow> Build();

        private:
            WindowConfig config_;
        };

    public:
        static std::shared_ptr<GuiWindow> GenerateButtonWindow(const std::vector<ButtonPair>& buttons);

    public:
        explicit GuiWindow(const WindowConfig& config_);

        ~GuiWindow();

        void UpdatePrimaryText(const char* text);
        void SetCustomRenderCallback(GuiWindowCustomRenderCallback callback);
        const GuiWindowCustomRenderCallback& GetCustomRenderCallback() const {
            return custom_render_callback_;
        }

        /**
         * Add a button
         * @param name The name that will be displayed on the button.
         * @param callback The callback that the application registers for this button.
         * This callback is triggered when the button is pressed.
         * @return The component id of the button.
         */
        int AddButton(const char* name, std::function<void()> callback);

        /**
         * Add a button with position
         * @param name The name that will be displayed on the button.
         * @param posX The x position(pixel) of the button.
         * @param posY The y position(pixel) of the button.
         * @param callback The callback that the application registers for this button.
         * This callback is triggered when the button is pressed.
         * @return The component id of the button.
         */
        int AddButton(const char* name, int posX, int posY, std::function<void()> callback);

        /**
         * Add a check box
         * @param name The name that will be displayed on the check box.
         * @param checked Variable bound to the check box state.
         * @return The component id of the check box.
         */
        int AddCheckBox(const char* name, bool* checked);

        /**
         * Add a check box
         * @param name The name that will be displayed on the check box.
         * @param posX The x position(pixel) of the button.
         * @param posY The y position(pixel) of the button.
         * @param checked Variable bound to the check box state.
         * @return The component id of the check box.
         */
        int AddCheckBox(const char* name, int posX, int posY, bool* checked);

        /**
         * Add a text
         * @param text A string describing the information.
         * @return The component id of the text.
         */
        int AddText(const char* text);

        /**
         * Add a text
         * @param text A string describing the information.
         * @param posX The x position(pixel) of the button.
         * @param posY The y position(pixel) of the button.
         * @return The component id of the text.
         */
        int AddText(const char* text, int posX, int posY);

        /**
         * Update text
         * @param componentId The component id of the text.
         * @param text A string describing the information.
         */
        void UpdateText(int componentId, const char* text);
        void SetButtonCallback(int componentId, std::function<void()> callback);

        /**
         * Set component position with pixel
         * @param componentId The component id of the text.
         * @param posX The x position(pixel) of the component.
         * @param posY The y position(pixel) of the component.
         */
        void SetComponentPos(int componentId, int posX, int posY);

        /**
         * Set component size with pixel
         * @param componentId The component id of the text.
         * @param width The width(pixel) of the component.
         * @param height The height(pixel) of the component.
         */
        void SetComponentSize(int componentId, int width, int height);

        /**
         * Set component background color
         * @param componentId The component id of the text.
         * @param r The red component of the component.
         * @param g The green component of the component.
         * @param b The blue component of the component.
         * @param a The alpha component of the component.
         */
        void SetComponentBgColor(int componentId, float r, float g, float b, float a);

        /**
         * Set component text color
         * @param componentId The component id of the text.
         * @param r The red component of the component.
         * @param g The green component of the component.
         * @param b The blue component of the component.
         * @param a The alpha component of the component.
         */
        void SetComponentTextColor(int componentId, float r, float g, float b, float a);

        /**
         * Set component text size
         * @param componentId The component id of the text.
         * @param size The size of the component.
         */
        void SetComponentTextSize(int componentId, int size);

        /**
         * injecting input event
         * @param x Normalized collision point coordinates, 0.0 ~ 1.0 from left to right
         * @param y Normalized collision point coordinates, 0.0 ~ 1.0 from bottom to top
         * @param bCollision Whether there is a collision point
         * @param bTrigger Whether the handle has a trigger on
         * @param side left is 0, right is 1
         */
        void InjectingRayInputEvent(float x, float y, bool bCollision, bool bTrigger, int side);

        const WindowConfig& GetConfig() {
            return config_;
        }

        std::unordered_map<int, WindowComponentConfig>& GetComponents() {
            return components_;
        }

        const CollisionState* GetCollisionState() {
            return collision_state_;
        }

        RayInputEvent GetCurrentRayInputEvent();

        bool IsRendered() {
            return need_render_;
        }

        void SetRendered(bool bRendered) {
            need_render_ = bRendered;
        }

        void SetTextureId(uint32_t texture) {
            rendered_tex_id_ = texture;
        }

        uint32_t GetTextureId() {
            return rendered_tex_id_;
        }

        float GetAspectRatio() {
            return static_cast<float>(config_.width) / static_cast<float>(config_.height);
        }

    private:
        WindowComponentConfig* ValidateAndGetComponent(int componentId);

    private:
        static constexpr int BUTTON_WIDTH{120};
        static constexpr int BUTTON_HEIGHT{40};
        static constexpr int BUTTON_INTERVAL_X{6};
        static constexpr int BUTTON_INTERVAL_Y{6};
        static constexpr int BUTTON_ROW_MAX{4};

        WindowConfig config_;
        std::unordered_map<int, WindowComponentConfig> components_;
        int component_count_{0};

        CollisionState collision_state_[2];
        RayInputEvent ray_input_;
        bool need_render_{true};
        uint32_t rendered_tex_id_{0};
        GuiWindowCustomRenderCallback custom_render_callback_{};
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_GUIWINDOW_H
