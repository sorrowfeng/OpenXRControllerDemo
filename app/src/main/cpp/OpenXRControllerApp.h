/*
 * OpenXR Controller Demo - Main Application Class
 */

#pragma once

// OpenXR platform defines must come before openxr_platform.h
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES

#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <memory>
#include <vector>
#include <array>
#include <string>

struct android_app;
class GraphicsRenderer;
class InputManager;

// Per-eye view info
struct EyeView {
    XrSwapchain swapchain = XR_NULL_HANDLE;
    std::vector<XrSwapchainImageOpenGLESKHR> images;
    std::vector<GLuint> framebuffers;  // FBOs for each swapchain image
    std::vector<GLuint> depthRenderbuffers;  // Depth buffer for each swapchain image
    uint32_t width = 0;
    uint32_t height = 0;
};

// Controller state
struct ControllerState {
    bool active = false;
    XrPosef gripPose;
    XrPosef aimPose;
    float triggerValue = 0.0f;
    float gripValue = 0.0f;
    XrVector2f thumbstick = {0.0f, 0.0f};
    bool btnA = false;
    bool btnB = false;
    bool btnX = false;
    bool btnY = false;
    bool btnMenu = false;
    bool btnThumbstick = false;
};

class OpenXRControllerApp {
public:
    OpenXRControllerApp();
    ~OpenXRControllerApp();

    // Initialize OpenXR (instance and system, doesn't need window)
    bool Initialize(struct android_app* app);

    // Initialize session (requires window) - call after SetNativeWindow
    bool InitializeSession();

    // Shutdown
    void Shutdown();

    // Set native window from Android (just stores pointer, doesn't initialize)
    void SetNativeWindow(ANativeWindow* window);

    // Check if window is pending initialization
    bool HasPendingWindow() const { return m_pendingWindow != nullptr; }

    // Initialize with pending window (call from main loop)
    bool InitializeWithWindow();

    // Set resumed state
    void SetResumed(bool resumed) { m_resumed = resumed; }

    // Check if resumed
    bool IsResumed() const { return m_resumed; }

    // Check if session is running
    bool IsSessionRunning() const { return m_sessionRunning; }

    // Check if initialized
    bool IsInitialized() const { return m_initialized; }

    // Update input and state
    void Update();

    // Render a frame
    void RenderFrame();

private:
    // EGL initialization
    bool InitializeEGL();

    // OpenXR initialization steps
    bool InitializeLoader();
    bool CreateInstance();
    bool GetSystem();
    bool CreateSession();
    bool CreateReferenceSpace();
    bool CreateSwapchains();
    bool CreateFramebuffers();  // Create FBOs for swapchain images

    // Poll OpenXR events
    void PollEvents();

    // Render a single view
    void RenderView(const XrCompositionLayerProjectionView& view, uint32_t eyeIndex);

    // Log controller state
    void LogControllerState(int hand, const ControllerState& state);

private:
    // Android
    struct android_app* m_androidApp = nullptr;
    ANativeWindow* m_nativeWindow = nullptr;
    bool m_resumed = false;

    // JNI global reference for Activity (needed for PICO OpenXR)
    jobject m_globalActivityRef = nullptr;

    // Pending window for deferred initialization
    ANativeWindow* m_pendingWindow = nullptr;

    // Initialization state
    bool m_initialized = false;
    bool m_instanceCreated = false;
    bool m_systemCreated = false;
    bool m_sessionCreated = false;

    // OpenXR handles
    XrInstance m_instance = XR_NULL_HANDLE;
    XrSystemId m_systemId = XR_NULL_SYSTEM_ID;
    XrSession m_session = XR_NULL_HANDLE;
    XrSpace m_referenceSpace = XR_NULL_HANDLE;
    XrSwapchain m_swapchain = XR_NULL_HANDLE;

    // View configuration
    XrViewConfigurationType m_viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    std::vector<XrViewConfigurationView> m_viewConfigViews;

    // Per-eye data
    std::array<EyeView, 2> m_eyeViews;

    // Session state
    XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;
    bool m_sessionRunning = false;
    bool m_shouldExit = false;

    // Frame timing
    XrTime m_predictedDisplayTime = 0;

    // Controllers
    std::array<ControllerState, 2> m_controllers;

    // Input manager
    std::unique_ptr<InputManager> m_inputManager;

    // Graphics renderer
    std::unique_ptr<GraphicsRenderer> m_renderer;

    // Extension support
    bool m_supportsHandTracking = false;
    bool m_supportsColorScaleBias = false;
};
