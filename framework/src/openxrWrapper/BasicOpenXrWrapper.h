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

#ifndef PICONATIVEOPENXRSAMPLES_BASICOPENXRWRAPPER_H
#define PICONATIVEOPENXRSAMPLES_BASICOPENXRWRAPPER_H

#include "graphicsPlugin/IXrGraphicsPlugin.h"
#include "platformPlugin/IXrPlatformPlugin.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include "util/Common.h"
#include "Configurations.h"
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <functional>
#include "ExtensionFeaturesManager.h"
#include "SampleCollisionDetector.h"

namespace PVRSampleFW {
    namespace Side {
        const int LEFT = 0;
        const int RIGHT = 1;
        const int COUNT = 2;
    }  // namespace Side

    struct InputState {
        XrActionSet action_set{XR_NULL_HANDLE};
        XrAction grab_action{XR_NULL_HANDLE};
        XrAction grip_pose_action{XR_NULL_HANDLE};
        XrAction aim_pose_action{XR_NULL_HANDLE};
        XrAction vibrate_action{XR_NULL_HANDLE};
        XrAction quit_action{XR_NULL_HANDLE};
        std::array<XrPath, Side::COUNT> controller_subaction_paths;
        std::array<XrSpace, Side::COUNT> controller_grip_spaces;
        std::array<XrSpace, Side::COUNT> controller_aim_spaces;
        std::array<float, Side::COUNT> controller_scales = {{1.0f, 1.0f}};
        std::array<XrBool32, Side::COUNT> controller_actives;

        /*************************pico controller******************/
        XrAction touchpad_action{XR_NULL_HANDLE};
        XrAction A_X_action{XR_NULL_HANDLE};
        XrAction home_action{XR_NULL_HANDLE};
        XrAction B_Y_action{XR_NULL_HANDLE};
        XrAction back_action{XR_NULL_HANDLE};
        XrAction side_action{XR_NULL_HANDLE};
        XrAction trigger_action{XR_NULL_HANDLE};
        XrAction joystick_action{XR_NULL_HANDLE};
        XrAction battery_action{XR_NULL_HANDLE};

        XrAction A_X_touch_action{XR_NULL_HANDLE};
        XrAction B_Y_touch_action{XR_NULL_HANDLE};
        XrAction rocker_touch_action{XR_NULL_HANDLE};
        XrAction trigger_touch_action{XR_NULL_HANDLE};
        XrAction thumb_rest_touch_action{XR_NULL_HANDLE};
        XrAction grip_action{XR_NULL_HANDLE};
        XrAction trigger_click_action{XR_NULL_HANDLE};

        XrAction A_action{XR_NULL_HANDLE};
        XrAction B_action{XR_NULL_HANDLE};
        XrAction X_action{XR_NULL_HANDLE};
        XrAction Y_action{XR_NULL_HANDLE};
        XrAction A_touch_action{XR_NULL_HANDLE};
        XrAction B_touch_action{XR_NULL_HANDLE};
        XrAction X_touch_action{XR_NULL_HANDLE};
        XrAction Y_touch_action{XR_NULL_HANDLE};
        /**************************merlineE******************************/
        XrAction trackpad_click_action{XR_NULL_HANDLE};
        XrAction trackpad_touch_action{XR_NULL_HANDLE};
        XrAction trackpad_value_action{XR_NULL_HANDLE};
        /*************************pico controller******************/

        //eye tracking
        XrAction gaze_action{XR_NULL_HANDLE};
        XrSpace gaze_action_space;
        XrBool32 gaze_active;
    };

    enum XrLoaderPlatformType {
        XR_LOADER_PLATFORM_ANDROID = 0,
        XR_LOADER_PLATFORM_WINDOWS,
        XR_LOADER_PLATFORM_LINUX,
        XR_LOADER_PLATFORM_UNKNOWN
    };

    struct XrFrameIn {
        /// frame accounting
        int64_t frame_number{-1};
        /// frame timing
        XrTime predicted_display_time;
        XrTime delta_time_in_seconds;
        XrTime previous_display_time;
        /// tracking
        XrPosef head_pose;
        XrPosef controller_poses[Side::COUNT];      // grip pose
        XrPosef controller_aim_poses[Side::COUNT];  // aim pose
        std::array<XrBool32, Side::COUNT> controller_actives{XR_FALSE, XR_FALSE};
        /// view info
        XrView views[Side::COUNT];
        /// input
        XrVector2f left_joystick_position = {0.0f, 0.0f};
        XrVector2f right_joystick_position = {0.0f, 0.0f};
        float controller_battery_value[Side::COUNT];  // 0-5
        float controller_trigger_value[Side::COUNT];  // 0-255 --> 0-1
        float controller_grip_value[Side::COUNT];     // 0-255 --> 0-1

        // buttons bool value in bitmask
        uint32_t all_buttons_bitmask{0};
        uint32_t all_touches_bitmask{0};
        uint32_t last_frame_all_buttons{0};
        uint32_t last_frame_all_touches{0};
        // const bitmask of button click actions
        static const int kButtonA = 1 << 0;
        static const int kButtonB = 1 << 1;
        static const int kButtonX = 1 << 2;
        static const int kButtonY = 1 << 3;
        static const int kButtonMenu = 1 << 4;  //menu button at left
        static const int kButtonHome = 1 << 5;  //home button at right
        static const int kButtonGripTriggerLeft = 1 << 6;
        static const int kButtonGripTriggerRight = 1 << 7;
        static const int kButtonTriggerLeft = 1 << 8;
        static const int kButtonTriggerRight = 1 << 9;
        static const int kButtonJoystickLeft = 1 << 10;
        static const int kButtonJoystickRight = 1 << 11;
        static const int kButtonBackLeft = 1 << 12;
        static const int kButtonBackRight = 1 << 13;
        static const int kButtonTouchpadLeft = 1 << 14;
        static const int kButtonTouchpadRight = 1 << 15;
        static const int kButtonSideLeft = 1 << 16;
        static const int kButtonSideRight = 1 << 17;
        // const bitmask of touch actions
        static const int kTouchJoystickLeft = 1 << 0;
        static const int kTouchJoystickRight = 1 << 1;
        static const int kTouchTriggerLeft = 1 << 2;
        static const int kTouchTriggerRight = 1 << 3;
        static const int kTouchThumbRestLeft = 1 << 4;
        static const int kTouchThumbRestRight = 1 << 5;
        static const int kTouchRockerLeft = 1 << 6;
        static const int kTouchRockerRight = 1 << 7;
        static const int kTouchX = 1 << 8;
        static const int kTouchY = 1 << 9;
        static const int kTouchA = 1 << 10;
        static const int kTouchB = 1 << 11;

        inline bool Clicked(const uint32_t& b) const {
            const bool isDown = (b & all_buttons_bitmask) != 0;
            const bool wasDown = (b & last_frame_all_buttons) != 0;
            return (wasDown && !isDown);
        }
        inline bool Touched(const uint32_t& t) const {
            const bool isDown = (t & all_touches_bitmask) != 0;
            const bool wasDown = (t & last_frame_all_touches) != 0;
            return (wasDown && !isDown);
        }
    };

    struct PxrControllerInputValue {
        XrVector2f joystick;      // 0-255
        int home_value;           // 0/1
        int back_value;           // 0/1
        int touchpad_value;       // 0/1
        int volume_up;            // 0/1
        int volume_down;          // 0/1
        float trigger_value;      // 0-255 --> 0-1
        int battery_value;        // 0-5
        int A_X_value;            // 0/1
        int B_Y_value;            // 0/1
        int side_value;           // 0/1
        float grip_value;         // 0-255  --> 0-1
        int trigger_click_value;  // 0/1
        int reserved_key_1;
        int reserved_key_2;
        int reserved_key_3;
        int reserved_key_4;

        int A_X_touch_value;         // 0/1
        int B_Y_touch_value;         // 0/1
        int rocker_touch_value;      // 0/1
        int trigger_touch_value;     // 0/1
        int thumb_rest_touch_value;  // 0/1
        int reserved_touch_0;
        int reserved_touch_1;
        int reserved_touch_2;
        int reserved_touch_3;
        int reserved_touch_4;
    };

    typedef enum {
        XR_NO_DEVICE_Type = 0,
        XR_HB_Controller_Type = 1,
        XR_CV_Controller_Type = 2,
        XR_HB2_Controller_Type = 3,
        XR_CV2_Controller_Type = 4,
        XR_CV3_Optics_Controller_Type = 5,
        XR_CV3_Phoenix_Controller_Type = 6,
        XR_CV3_MerlinE_Controller_Type = 7,
        XR_CV3_Hawk_Controller_Type = 8
    } XrControllerTypePICO;

    struct Swapchain {
        XrSwapchain handle;
        int32_t width;
        int32_t height;
    };

    union XrCompositionLayerUnion {
        XrCompositionLayerProjection projection;
        XrCompositionLayerQuad quad;
        XrCompositionLayerCylinderKHR cylinder;
        XrCompositionLayerCubeKHR cube;
        XrCompositionLayerEquirectKHR equirect;
        XrCompositionLayerPassthroughFB passthrough;
    };

    using EventHandlerFunc =
            std::function<void(class BasicOpenXrWrapper* openxr, const XrEventDataBaseHeader* eventData,
                               bool* exitRenderLoop, bool* requestRestart)>;

    struct XrEventHandler {
        XrStructureType event_type{XR_TYPE_UNKNOWN};

        EventHandlerFunc handler{nullptr};

        bool operator<(const XrEventHandler& other) const {
            return event_type < other.event_type;
        }

        bool operator==(const XrEventHandler& other) const {
            return event_type == other.event_type;
        }
    };

    using InputHandlerFunc =
            std::function<void(class BasicOpenXrWrapper* openxr, const PVRSampleFW::XrFrameIn& frameIn)>;

    /// Constants
    static const int MAX_NUM_COMPOSITION_LAYERS = 16;
    static const int MAX_NUM_EYES = 2;

    class BasicOpenXrWrapper {
    public:
        BasicOpenXrWrapper(const std::shared_ptr<IXrGraphicsPlugin>& ghp, const std::shared_ptr<IXrPlatformPlugin>& ppp,
                           const bool& useInput = true)
            : platform_plugin_(ppp), graphics_plugin_(ghp), use_input_handling_(useInput) {
        }

        BasicOpenXrWrapper() = default;
        virtual ~BasicOpenXrWrapper();

        virtual void InitializeLoader(XrLoaderPlatformType loaderType, void* XR_MAY_ALIAS applicationVM,
                                      void* XR_MAY_ALIAS applicationContext);

        virtual bool InitializeInstance(std::string applicationName);

        virtual bool InitializeSystem();

        virtual bool InitializeSession();

        virtual bool InitializeSwapchains();

        virtual bool DestroySession();

        virtual bool DestroyInstance();

        virtual bool GetConfigFromConfigurations(const Configurations& configurations);

        virtual void HandleXrEvents(bool* exitRenderLoop, bool* requestRestart);

        virtual bool RegisterHandleInputFunc(InputHandlerFunc handleInputFunc);

        virtual void DoFrame();

        virtual bool CheckBlendMode(XrEnvironmentBlendMode blendMode);

        virtual bool IsSessionRunning() const;

        virtual bool IsSessionFocused() const;

        virtual bool IsExtensionSupported(const char* extensionName) const;

        virtual bool IsExtensionEnabled(const char* extensionName) const;

        virtual void HandleInteractionProfileChangedEvent(void* userData, const XrEventDataBaseHeader* eventData,
                                                          bool* exitRenderLoop, bool* requestRestart);

        virtual void HandleSessionStateChangeEvent(void* userData, const XrEventDataBaseHeader* eventData,
                                                   bool* exitRenderLoop, bool* requestRestart);

    public:
        XrInstance GetXrInstance() const {
            return xr_instance_;
        }

        XrSystemId GetXrSystemId() const {
            return xr_system_id_;
        }

        XrSession GetXrSession() const {
            return xr_session_;
        }

        XrSpace GetAppSpace() const {
            return app_space_;
        }

        XrSpace GetHeadSpace() const {
            return head_view_space_;
        }

        const InputState& GetInputState() const {
            return input_;
        }

        const XrFrameIn& GetCurrentFrameIn() const {
            return current_frame_in_;
        }

        float GetTargetDispalyRefreshRate() const {
            return target_display_refresh_rate;
        }

        void RegisterXrEventHandler(XrEventHandler handler);

        bool RegisterExtensionFeature(const std::shared_ptr<IOpenXRExtensionPlugin> modularFeature);

    private:
        virtual void PollActions();

        virtual void RenderFrame();

        virtual bool InitializeActions();

        virtual void GetSystemProperties();

        virtual void InitializeViewConfiguration();

    protected:
        /// @brief Assemble the various features you want here,
        ///  including extensions and some others
        /// @note: optional
        virtual void CustomizedExtensionAndFeaturesInit();
        /// @brief Do what you want to do in the initialization of the session.
        /// Customized functions for subclass, called by InitializeSession
        /// @return true if success, false if failed
        /// @note: optional
        // for example, you can create your own visualized  reference space for objects you
        // rendered
        virtual bool CustomizedSessionInit() {
            return true;
        }

        /// @brief Do what you want to do in the destruction of the session.
        /// Customized functions for subclass, called by DestroySession
        /// @return true if success, false if failed
        /// @note: optional
        virtual void CustomizedSessionDestroy() {
        }

        //// @brief Initialize the action set and actions.
        /// Customized functions for subclass, called by InitializeActions
        /// @return true if success, false if failed
        /// @note: optional
        // for example, you can create your own action set and actions
        // for your app, and bind them to the interaction profile of the user.
        // must be called during initializeSession, and after session is created
        virtual bool CustomizedActionsInit() {
            return true;
        }

        /// @brief Poll the customized actions.
        /// Customized functions for subclass, called by PollActions
        /// @return true if success, false if failed
        /// @note: optional
        // for example, you can poll your own actions, and update your own
        // input state corresponding to the action.
        virtual bool CustomizedActionsPoll() {
            return true;
        }

        /// @brief Do what you want to do in the initialization of the swapchains_.
        /// Customized functions for subclass, called by InitializeSwapchains
        /// @return true if success, false if failed
        /// @note: optional, suggested
        // for example, you can create your own swapchains_, and bind them to the
        // view configurations of the system.
        virtual bool CustomizedSwapchainsInit() {
            return true;
        }

        /// @brief Register your own event handlers in this function.
        /// Customized functions for subclass, called by InitializeXrEventHandler
        /// @return true if success, false if failed
        /// @note: optional
        // for example, you can register your own event handlers, such as
        // XR_EVENT_TYPE_INTERACTION_PROFILE_CHANGED, XR_EVENT_TYPE_SESSION_STATE_CHANGED,
        virtual bool CustomizedXrEventHandlerSetup() {
            return true;
        }

        /// @brief Register your own input handlers in this function.
        /// Customized functions for subclass
        /// @return true if success, false if failed
        /// @note: optional
        // for example, you can register your own event handlers, such as
        virtual bool CustomizedXrInputHandlerSetup() {
            return true;
        }

        /// @brief Do what you want to do before waiting for the frame.
        /// Customized functions for subclass, called by DoFrame
        /// @return true if success, false if failed
        /// @note: optional
        virtual bool CustomizedPreWaitFrame(XrFrameWaitInfo&, XrFrameState&) {
            return true;
        }

        /// @brief Do what you want to do after waiting for the frame.
        /// Customized functions for subclass, called by DoFrame
        /// @return true if success, false if failed
        /// @note: optional
        virtual bool CustomizedPostWaitFrame(XrFrameWaitInfo&, XrFrameState&) {
            return true;
        }

        /// @brief Do what you want to do before rendering the frame.
        /// Customized functions for subclass, called by DoFrame
        /// @return true if success, false if failed
        /// @note: optional
        // for example, you can do something before rendering the frame, such as
        // updating the input state.
        virtual bool CustomizedPreRenderFrame() {
            return true;
        }

        /// @brief Do what you want to do after rendering the frame.
        /// Customized functions for subclass, called by DoFrame
        /// @return true if success, false if failed
        /// @note: optional
        // for example, you can do something after rendering the frame, such as
        // submitting the frame.
        virtual bool CustomizedPostRenderFrame() {
            return true;
        }

        /// @brief Customized rendering function.
        /// Customized functions for subclass, called by DoFrame
        /// @return true if success, false if failed
        /// @note: must
        // for example, you can render your own scenes_, such as
        // rendering the scene with the swapchains_.
        virtual bool CustomizedRender() {
            return true;
        }

        /// @brief Put your own input handling logic here.
        /// Customized functions for subclass, called by PollActions
        /// @return true if success, false if failed
        /// @note: optional, you can choose input callbacks, @see RegisterHandleInputFunc
        // for example, you can handle your own input, such as
        // handling the input of the controller.
        virtual bool CustomizedHandleInput() {
            return true;
        }

        /// @brief Design your own session change logic here.
        /// Customized functions for subclass, called by HandleSessionStateChangeEvent
        /// @return true if success, false if failed
        virtual bool CustomizedSessionStateChange(XrSessionState state) {
            xr_session_state_ = state;
            return true;
        }

        /// @brief Get the app space create info you want.
        /// Customized functions for subclass, called by InitializeSession
        /// @return true if success, false if failed
        /// @note: optional
        // for example, you can create your own app space, such as
        // creating a stage space.
        virtual XrReferenceSpaceCreateInfo CustomizedGetAppSpaceCreateInfo();

    protected:  /// convenient be used by subclass
        void GetLayersAndExtensions();

        void LogInstanceInfo();

        void LogViewConfigurations();

        void LogEnvironmentBlendMode(XrViewConfigurationType type);

        void LogReferenceSpaces();

        void LogActionSourceName(XrAction action, const std::string& actionName) const;

        inline std::string GetXrVersionString(XrVersion ver) {
            return Fmt("%d.%d.%d", XR_VERSION_MAJOR(ver), XR_VERSION_MINOR(ver), XR_VERSION_PATCH(ver));
        }

        XrActionSet CreateActionSet(uint32_t priority, const char* name, const char* localizedName);

        XrAction CreateAction(XrActionSet actionSet, XrActionType type, const char* actionName,
                              const char* localizedName, int countSubactionPaths, XrPath* subactionPaths);

        XrActionSuggestedBinding ActionSuggestedBinding(XrAction action, XrPath bindingPath);

        void SuggestInteractionProfileBindings(
                const std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> allSuggestedBindings);

        XrSpace CreateActionSpace(XrAction poseAction, XrPath subactionPath);

        void InitializeXrEventHandler();

        // Return event if one is available, otherwise return null.
        const XrEventDataBaseHeader* TryReadNextEvent();

        int GetControllerType();

    protected:  /// convenient be used by subclass
        std::shared_ptr<IXrPlatformPlugin> platform_plugin_;
        std::shared_ptr<IXrGraphicsPlugin> graphics_plugin_;
        XrInstance xr_instance_{XR_NULL_HANDLE};
        XrSystemId xr_system_id_{XR_NULL_SYSTEM_ID};
        XrSession xr_session_{XR_NULL_HANDLE};

        // extension
        std::vector<std::string> enabled_extensions_;
        std::vector<std::string> non_plugin_extensions_;
        std::vector<XrExtensionProperties> runtime_supported_extensions_;
        std::unique_ptr<ExtensionFeaturesManager> extension_features_manager_;

        // space
        XrSpace app_space_{XR_NULL_HANDLE};
        XrReferenceSpaceType app_space_type_{XR_REFERENCE_SPACE_TYPE_LOCAL};
        XrSpace head_view_space_{XR_NULL_HANDLE};

        // input
        PVRSampleFW::InputState input_{};
        PVRSampleFW::PxrControllerInputValue controller_input_value_[Side::COUNT];
        uint32_t last_frame_all_buttons_{0u};
        uint32_t last_frame_all_touches_{0u};
        int controller_type_{Simple_Controller};
        /// TODO: add eye tracking check implementation
        bool eye_tracking_supported_{false};
        bool use_input_handling_{true};

        XrSessionState xr_session_state_{XR_SESSION_STATE_UNKNOWN};
        bool is_session_running_{false};
        bool is_stage_bound_dirty_{true};

        XrFormFactor xr_form_factor_{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
        float target_display_refresh_rate{90.0f};

        XrSystemProperties system_properties_{XR_TYPE_SYSTEM_PROPERTIES};
        XrViewConfigurationType config_view_config_type_{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
        std::vector<XrViewConfigurationView> config_views_;
        std::vector<XrView> projection_views_cache_;

        XrEnvironmentBlendMode xr_environment_blend_mode_{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};
        std::set<XrEnvironmentBlendMode> acceptable_blend_modes_;

        std::vector<Swapchain> swapchains_;
        std::map<XrSwapchain, std::vector<XrSwapchainImageBaseHeader*>> swapchain_images_;
        int64_t color_swapchain_format_{-1};

        XrEventDataBuffer event_data_buffer_;
        std::vector<XrEventHandler> registered_event_handlers_;

        PVRSampleFW::XrFrameIn current_frame_in_;
        std::vector<PVRSampleFW::InputHandlerFunc> input_callbacks_;

        // composition section
        uint32_t comp_layer_count_{0};
        PVRSampleFW::XrCompositionLayerUnion comp_layers_[MAX_NUM_COMPOSITION_LAYERS];
        std::vector<XrCompositionLayerProjectionView> projection_layer_views_;
    };
}  // namespace PVRSampleFW
#endif  //PICONATIVEOPENXRSAMPLES_BASICOPENXRWRAPPER_H
