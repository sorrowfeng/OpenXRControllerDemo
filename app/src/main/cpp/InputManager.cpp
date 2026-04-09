/*
 * OpenXR Controller Demo - Input Manager Implementation
 */

#include "InputManager.h"
#include <android/log.h>
#include <cstring>

#define LOG_TAG "InputManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

InputManager::InputManager() = default;

InputManager::~InputManager() {
    // Cleanup action spaces
    for (int i = 0; i < 2; i++) {
        if (m_aimSpaces[i] != XR_NULL_HANDLE) {
            xrDestroySpace(m_aimSpaces[i]);
        }
        if (m_gripSpaces[i] != XR_NULL_HANDLE) {
            xrDestroySpace(m_gripSpaces[i]);
        }
    }

    // Cleanup action set
    if (m_actionSet != XR_NULL_HANDLE) {
        xrDestroyActionSet(m_actionSet);
    }
}

bool InputManager::Initialize(XrInstance instance, XrSession session) {
    m_instance = instance;
    m_session = session;

    LOGI("Initializing input manager...");

    // Get hand paths
    xrStringToPath(m_instance, "/user/hand/left", &m_handPaths[0]);
    xrStringToPath(m_instance, "/user/hand/right", &m_handPaths[1]);

    if (!CreateActionSet()) return false;
    if (!CreateActions()) return false;
    if (!CreateActionSpaces()) return false;
    if (!SuggestBindings()) return false;
    if (!AttachActionSet()) return false;

    m_initialized = true;
    LOGI("Input manager initialized successfully");
    return true;
}

bool InputManager::CreateActionSet() {
    XrActionSetCreateInfo actionSetInfo = {
        XR_TYPE_ACTION_SET_CREATE_INFO,
        nullptr,
        "controller_input",
        "Controller Input",
        0
    };

    XrResult result = xrCreateActionSet(m_instance, &actionSetInfo, &m_actionSet);
    if (XR_FAILED(result)) {
        LOGE("xrCreateActionSet failed: %d", result);
        return false;
    }

    return true;
}

bool InputManager::CreateActions() {
    // Aim pose action
    XrActionCreateInfo aimPoseActionInfo = {
        XR_TYPE_ACTION_CREATE_INFO,
        nullptr,
        "aim_pose",
        XR_ACTION_TYPE_POSE_INPUT,
        2,
        m_handPaths,
        "Aim Pose"
    };
    xrCreateAction(m_actionSet, &aimPoseActionInfo, &m_aimPoseAction);

    // Grip pose action
    XrActionCreateInfo gripPoseActionInfo = {
        XR_TYPE_ACTION_CREATE_INFO,
        nullptr,
        "grip_pose",
        XR_ACTION_TYPE_POSE_INPUT,
        2,
        m_handPaths,
        "Grip Pose"
    };
    xrCreateAction(m_actionSet, &gripPoseActionInfo, &m_gripPoseAction);

    // Trigger action
    XrActionCreateInfo triggerActionInfo = {
        XR_TYPE_ACTION_CREATE_INFO,
        nullptr,
        "trigger",
        XR_ACTION_TYPE_FLOAT_INPUT,
        2,
        m_handPaths,
        "Trigger"
    };
    xrCreateAction(m_actionSet, &triggerActionInfo, &m_triggerAction);

    // Grip action
    XrActionCreateInfo gripActionInfo = {
        XR_TYPE_ACTION_CREATE_INFO,
        nullptr,
        "grip",
        XR_ACTION_TYPE_FLOAT_INPUT,
        2,
        m_handPaths,
        "Grip"
    };
    xrCreateAction(m_actionSet, &gripActionInfo, &m_gripAction);

    // Thumbstick action
    XrActionCreateInfo thumbstickActionInfo = {
        XR_TYPE_ACTION_CREATE_INFO,
        nullptr,
        "thumbstick",
        XR_ACTION_TYPE_VECTOR2F_INPUT,
        2,
        m_handPaths,
        "Thumbstick"
    };
    xrCreateAction(m_actionSet, &thumbstickActionInfo, &m_thumbstickAction);

    // Button A action (right hand)
    XrPath rightHandPath = m_handPaths[1];
    XrActionCreateInfo btnAActionInfo = {
        XR_TYPE_ACTION_CREATE_INFO,
        nullptr,
        "button_a",
        XR_ACTION_TYPE_BOOLEAN_INPUT,
        1,
        &rightHandPath,
        "Button A"
    };
    xrCreateAction(m_actionSet, &btnAActionInfo, &m_btnAAction);

    // Button B action (right hand)
    XrActionCreateInfo btnBActionInfo = {
        XR_TYPE_ACTION_CREATE_INFO,
        nullptr,
        "button_b",
        XR_ACTION_TYPE_BOOLEAN_INPUT,
        1,
        &rightHandPath,
        "Button B"
    };
    xrCreateAction(m_actionSet, &btnBActionInfo, &m_btnBAction);

    // Button X action (left hand)
    XrPath leftHandPath = m_handPaths[0];
    XrActionCreateInfo btnXActionInfo = {
        XR_TYPE_ACTION_CREATE_INFO,
        nullptr,
        "button_x",
        XR_ACTION_TYPE_BOOLEAN_INPUT,
        1,
        &leftHandPath,
        "Button X"
    };
    xrCreateAction(m_actionSet, &btnXActionInfo, &m_btnXAction);

    // Button Y action (left hand)
    XrActionCreateInfo btnYActionInfo = {
        XR_TYPE_ACTION_CREATE_INFO,
        nullptr,
        "button_y",
        XR_ACTION_TYPE_BOOLEAN_INPUT,
        1,
        &leftHandPath,
        "Button Y"
    };
    xrCreateAction(m_actionSet, &btnYActionInfo, &m_btnYAction);

    // Menu button action (left hand)
    XrActionCreateInfo btnMenuActionInfo = {
        XR_TYPE_ACTION_CREATE_INFO,
        nullptr,
        "button_menu",
        XR_ACTION_TYPE_BOOLEAN_INPUT,
        1,
        &leftHandPath,
        "Menu Button"
    };
    xrCreateAction(m_actionSet, &btnMenuActionInfo, &m_btnMenuAction);

    // Thumbstick click action
    XrActionCreateInfo btnThumbstickActionInfo = {
        XR_TYPE_ACTION_CREATE_INFO,
        nullptr,
        "thumbstick_click",
        XR_ACTION_TYPE_BOOLEAN_INPUT,
        2,
        m_handPaths,
        "Thumbstick Click"
    };
    xrCreateAction(m_actionSet, &btnThumbstickActionInfo, &m_btnThumbstickAction);

    // Haptic output action
    XrActionCreateInfo hapticActionInfo = {
        XR_TYPE_ACTION_CREATE_INFO,
        nullptr,
        "haptic",
        XR_ACTION_TYPE_VIBRATION_OUTPUT,
        2,
        m_handPaths,
        "Haptic"
    };
    xrCreateAction(m_actionSet, &hapticActionInfo, &m_hapticAction);

    LOGI("Actions created");
    return true;
}

bool InputManager::CreateActionSpaces() {
    for (int i = 0; i < 2; i++) {
        // Create aim space
        XrActionSpaceCreateInfo aimSpaceInfo = {
            XR_TYPE_ACTION_SPACE_CREATE_INFO,
            nullptr,
            m_aimPoseAction,
            m_handPaths[i],
            {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}}
        };
        xrCreateActionSpace(m_session, &aimSpaceInfo, &m_aimSpaces[i]);

        // Create grip space
        XrActionSpaceCreateInfo gripSpaceInfo = {
            XR_TYPE_ACTION_SPACE_CREATE_INFO,
            nullptr,
            m_gripPoseAction,
            m_handPaths[i],
            {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}}
        };
        xrCreateActionSpace(m_session, &gripSpaceInfo, &m_gripSpaces[i]);
    }

    LOGI("Action spaces created");
    return true;
}

bool InputManager::SuggestBindings() {
    // Get interaction profile path
    XrPath picoTouchPath;
    xrStringToPath(m_instance, "/interaction_profiles/pico/neo3_controller", &picoTouchPath);

    // Alternative: Try generic touch controller
    // xrStringToPath(m_instance, "/interaction_profiles/oculus/touch_controller", &picoTouchPath);

    std::vector<XrActionSuggestedBinding> bindings;

    // Aim pose
    for (int i = 0; i < 2; i++) {
        const char* path = (i == 0) ? "/user/hand/left/input/aim/pose" : "/user/hand/right/input/aim/pose";
        XrPath bindingPath;
        xrStringToPath(m_instance, path, &bindingPath);
        bindings.push_back({m_aimPoseAction, bindingPath});
    }

    // Grip pose
    for (int i = 0; i < 2; i++) {
        const char* path = (i == 0) ? "/user/hand/left/input/grip/pose" : "/user/hand/right/input/grip/pose";
        XrPath bindingPath;
        xrStringToPath(m_instance, path, &bindingPath);
        bindings.push_back({m_gripPoseAction, bindingPath});
    }

    // Trigger
    for (int i = 0; i < 2; i++) {
        const char* path = (i == 0) ? "/user/hand/left/input/trigger/value" : "/user/hand/right/input/trigger/value";
        XrPath bindingPath;
        xrStringToPath(m_instance, path, &bindingPath);
        bindings.push_back({m_triggerAction, bindingPath});
    }

    // Grip
    for (int i = 0; i < 2; i++) {
        const char* path = (i == 0) ? "/user/hand/left/input/squeeze/value" : "/user/hand/right/input/squeeze/value";
        XrPath bindingPath;
        xrStringToPath(m_instance, path, &bindingPath);
        bindings.push_back({m_gripAction, bindingPath});
    }

    // Thumbstick
    for (int i = 0; i < 2; i++) {
        const char* path = (i == 0) ? "/user/hand/left/input/thumbstick" : "/user/hand/right/input/thumbstick";
        XrPath bindingPath;
        xrStringToPath(m_instance, path, &bindingPath);
        bindings.push_back({m_thumbstickAction, bindingPath});
    }

    // A/B buttons (right hand)
    {
        XrPath bindingPath;
        xrStringToPath(m_instance, "/user/hand/right/input/a/click", &bindingPath);
        bindings.push_back({m_btnAAction, bindingPath});

        xrStringToPath(m_instance, "/user/hand/right/input/b/click", &bindingPath);
        bindings.push_back({m_btnBAction, bindingPath});
    }

    // X/Y buttons (left hand)
    {
        XrPath bindingPath;
        xrStringToPath(m_instance, "/user/hand/left/input/x/click", &bindingPath);
        bindings.push_back({m_btnXAction, bindingPath});

        xrStringToPath(m_instance, "/user/hand/left/input/y/click", &bindingPath);
        bindings.push_back({m_btnYAction, bindingPath});

        // Menu button on PICO might use different path - try system menu
        XrPath menuBindingPath;
        xrStringToPath(m_instance, "/user/hand/left/input/system/click", &menuBindingPath);
        bindings.push_back({m_btnMenuAction, menuBindingPath});
    }

    // Thumbstick click
    for (int i = 0; i < 2; i++) {
        const char* path = (i == 0) ? "/user/hand/left/input/thumbstick/click" : "/user/hand/right/input/thumbstick/click";
        XrPath bindingPath;
        xrStringToPath(m_instance, path, &bindingPath);
        bindings.push_back({m_btnThumbstickAction, bindingPath});
    }

    // Haptic
    for (int i = 0; i < 2; i++) {
        const char* path = (i == 0) ? "/user/hand/left/output/haptic" : "/user/hand/right/output/haptic";
        XrPath bindingPath;
        xrStringToPath(m_instance, path, &bindingPath);
        bindings.push_back({m_hapticAction, bindingPath});
    }

    XrInteractionProfileSuggestedBinding suggestedBindings = {
        XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING,
        nullptr,
        picoTouchPath,
        static_cast<uint32_t>(bindings.size()),
        bindings.data()
    };

    XrResult result = xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings);
    if (XR_FAILED(result)) {
        LOGE("xrSuggestInteractionProfileBindings failed: %d", result);
        // Try alternative interaction profile
        XrPath genericPath;
        xrStringToPath(m_instance, "/interaction_profiles/oculus/touch_controller", &genericPath);
        suggestedBindings.interactionProfile = genericPath;
        result = xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings);
        if (XR_FAILED(result)) {
            LOGE("Alternative binding also failed: %d", result);
            return false;
        }
    }

    LOGI("Bindings suggested successfully");
    return true;
}

bool InputManager::AttachActionSet() {
    XrSessionActionSetsAttachInfo attachInfo = {
        XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO,
        nullptr,
        1,
        &m_actionSet
    };

    XrResult result = xrAttachSessionActionSets(m_session, &attachInfo);
    if (XR_FAILED(result)) {
        LOGE("xrAttachSessionActionSets failed: %d", result);
        return false;
    }

    LOGI("Action set attached");
    return true;
}

void InputManager::Update(XrSession session, XrTime predictedDisplayTime, XrSpace referenceSpace) {
    if (!m_initialized || referenceSpace == XR_NULL_HANDLE) return;

    // Sync actions
    XrActiveActionSet activeActionSet = {m_actionSet, XR_NULL_PATH};
    XrActionsSyncInfo syncInfo = {
        XR_TYPE_ACTIONS_SYNC_INFO,
        nullptr,
        1,
        &activeActionSet
    };

    xrSyncActions(session, &syncInfo);

    // Update each hand
    for (int i = 0; i < 2; i++) {
        UpdateControllerState(i, predictedDisplayTime, referenceSpace);
    }
}

void InputManager::UpdateControllerState(int hand, XrTime time, XrSpace referenceSpace) {
    ControllerState& state = m_controllerStates[hand];

    // Get aim pose - use reference space as base space
    XrSpaceLocation aimLocation = {XR_TYPE_SPACE_LOCATION};
    XrResult result = xrLocateSpace(m_aimSpaces[hand], referenceSpace, time, &aimLocation);

    static int logCounter = 0;
    bool shouldLog = (++logCounter % 120 == 0); // Log every ~2 seconds

    if (shouldLog) {
        const char* handName = (hand == 0) ? "left" : "right";
        LOGI("Hand %s: xrLocateSpace result=%d, locationFlags=0x%X",
             handName, result, aimLocation.locationFlags);
    }

    if (XR_SUCCEEDED(result) && (aimLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
        state.aimPose = aimLocation.pose;
        state.active = true;
    } else {
        state.active = false;
    }

    // Get grip pose - use reference space as base space
    XrSpaceLocation gripLocation = {XR_TYPE_SPACE_LOCATION};
    XrResult gripResult = xrLocateSpace(m_gripSpaces[hand], referenceSpace, time, &gripLocation);
    if (XR_SUCCEEDED(gripResult) && (gripLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
        state.gripPose = gripLocation.pose;
    }

    // Get trigger value
    XrActionStateFloat triggerState = {XR_TYPE_ACTION_STATE_FLOAT};
    XrActionStateGetInfo triggerGetInfo = {
        XR_TYPE_ACTION_STATE_GET_INFO,
        nullptr,
        m_triggerAction,
        m_handPaths[hand]
    };
    xrGetActionStateFloat(m_session, &triggerGetInfo, &triggerState);
    if (triggerState.isActive) {
        state.triggerValue = triggerState.currentState;
    }

    // Get grip value
    XrActionStateFloat gripState = {XR_TYPE_ACTION_STATE_FLOAT};
    XrActionStateGetInfo gripGetInfo = {
        XR_TYPE_ACTION_STATE_GET_INFO,
        nullptr,
        m_gripAction,
        m_handPaths[hand]
    };
    xrGetActionStateFloat(m_session, &gripGetInfo, &gripState);
    if (gripState.isActive) {
        state.gripValue = gripState.currentState;
    }

    // Get thumbstick
    XrActionStateVector2f thumbstickState = {XR_TYPE_ACTION_STATE_VECTOR2F};
    XrActionStateGetInfo thumbstickGetInfo = {
        XR_TYPE_ACTION_STATE_GET_INFO,
        nullptr,
        m_thumbstickAction,
        m_handPaths[hand]
    };
    xrGetActionStateVector2f(m_session, &thumbstickGetInfo, &thumbstickState);
    if (thumbstickState.isActive) {
        state.thumbstick = thumbstickState.currentState;
    }

    // Get thumbstick click
    XrActionStateBoolean thumbstickClickState = {XR_TYPE_ACTION_STATE_BOOLEAN};
    XrActionStateGetInfo thumbstickClickGetInfo = {
        XR_TYPE_ACTION_STATE_GET_INFO,
        nullptr,
        m_btnThumbstickAction,
        m_handPaths[hand]
    };
    xrGetActionStateBoolean(m_session, &thumbstickClickGetInfo, &thumbstickClickState);
    if (thumbstickClickState.isActive) {
        state.btnThumbstick = thumbstickClickState.currentState;
    }

    // Get buttons (only for correct hands)
    if (hand == 1) { // Right hand - A/B buttons
        XrActionStateBoolean btnState = {XR_TYPE_ACTION_STATE_BOOLEAN};
        XrActionStateGetInfo btnGetInfo = {
            XR_TYPE_ACTION_STATE_GET_INFO,
            nullptr,
            m_btnAAction,
            m_handPaths[hand]
        };
        xrGetActionStateBoolean(m_session, &btnGetInfo, &btnState);
        if (btnState.isActive) {
            state.btnA = btnState.currentState;
        }

        btnGetInfo.action = m_btnBAction;
        xrGetActionStateBoolean(m_session, &btnGetInfo, &btnState);
        if (btnState.isActive) {
            state.btnB = btnState.currentState;
        }
    } else { // Left hand - X/Y/Menu buttons
        XrActionStateBoolean btnState = {XR_TYPE_ACTION_STATE_BOOLEAN};
        XrActionStateGetInfo btnGetInfo = {
            XR_TYPE_ACTION_STATE_GET_INFO,
            nullptr,
            m_btnXAction,
            m_handPaths[hand]
        };
        xrGetActionStateBoolean(m_session, &btnGetInfo, &btnState);
        if (btnState.isActive) {
            state.btnX = btnState.currentState;
        }

        btnGetInfo.action = m_btnYAction;
        xrGetActionStateBoolean(m_session, &btnGetInfo, &btnState);
        if (btnState.isActive) {
            state.btnY = btnState.currentState;
        }

        btnGetInfo.action = m_btnMenuAction;
        xrGetActionStateBoolean(m_session, &btnGetInfo, &btnState);
        if (btnState.isActive) {
            state.btnMenu = btnState.currentState;
        }
    }
}

ControllerState InputManager::GetControllerState(int hand) const {
    if (hand >= 0 && hand < 2) {
        return m_controllerStates[hand];
    }
    return ControllerState();
}
