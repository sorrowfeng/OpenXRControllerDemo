/*
 * OpenXR Controller Demo - Input Manager
 * Handles controller input using OpenXR action sets
 */

#pragma once

#include <openxr/openxr.h>
#include "OpenXRControllerApp.h"

class InputManager {
public:
    InputManager();
    ~InputManager();

    // Initialize input system
    bool Initialize(XrInstance instance, XrSession session);

    // Update input state
    void Update(XrSession session, XrTime predictedDisplayTime, XrSpace referenceSpace);

    // Get controller state for a hand (0=left, 1=right)
    ControllerState GetControllerState(int hand) const;

private:
    bool CreateActionSet();
    bool CreateActions();
    bool CreateActionSpaces();
    bool SuggestBindings();
    bool AttachActionSet();

    void UpdateControllerState(int hand, XrTime time, XrSpace referenceSpace);

private:
    XrInstance m_instance = XR_NULL_HANDLE;
    XrSession m_session = XR_NULL_HANDLE;

    // Action set
    XrActionSet m_actionSet = XR_NULL_HANDLE;

    // Actions
    XrAction m_aimPoseAction = XR_NULL_HANDLE;
    XrAction m_gripPoseAction = XR_NULL_HANDLE;
    XrAction m_triggerAction = XR_NULL_HANDLE;
    XrAction m_gripAction = XR_NULL_HANDLE;
    XrAction m_thumbstickAction = XR_NULL_HANDLE;
    XrAction m_btnAAction = XR_NULL_HANDLE;
    XrAction m_btnBAction = XR_NULL_HANDLE;
    XrAction m_btnXAction = XR_NULL_HANDLE;
    XrAction m_btnYAction = XR_NULL_HANDLE;
    XrAction m_btnMenuAction = XR_NULL_HANDLE;
    XrAction m_btnThumbstickAction = XR_NULL_HANDLE;
    XrAction m_hapticAction = XR_NULL_HANDLE;

    // Action spaces for each hand
    XrSpace m_aimSpaces[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
    XrSpace m_gripSpaces[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};

    // Subaction paths for left/right hands
    XrPath m_handPaths[2];

    // Cached controller states
    ControllerState m_controllerStates[2];

    bool m_initialized = false;
};
