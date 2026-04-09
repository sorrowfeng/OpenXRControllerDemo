/*
 * OpenXR Controller Demo - Main Application Implementation
 */

#include "OpenXRControllerApp.h"
#include "GraphicsRenderer.h"
#include "InputManager.h"
#include <android/log.h>
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cmath>
#include <unistd.h>

#define LOG_TAG "OpenXRControllerDemo"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// OpenXR function pointers
PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = nullptr;
PFN_xrCreateSwapchainAndroidSurfaceKHR pfnCreateSwapchainAndroidSurfaceKHR = nullptr;

OpenXRControllerApp::OpenXRControllerApp() = default;

OpenXRControllerApp::~OpenXRControllerApp() {
    Shutdown();
}

bool OpenXRControllerApp::Initialize(struct android_app* app) {
    m_androidApp = app;

    LOGI("=================================");
    LOGI("OpenXR Controller Demo Starting");
    LOGI("=================================");

    // Initialize OpenXR loader
    if (!InitializeLoader()) {
        LOGE("Failed to initialize OpenXR loader");
        return false;
    }

    // Create OpenXR instance
    if (!CreateInstance()) {
        LOGE("Failed to create OpenXR instance");
        return false;
    }

    // Get system
    if (!GetSystem()) {
        LOGE("Failed to get OpenXR system");
        return false;
    }

    LOGI("Instance/System initialized, waiting for native window...");
    return true;
}

bool OpenXRControllerApp::InitializeSession() {
    if (m_sessionCreated) {
        LOGI("Session already created");
        return true;
    }

    if (m_nativeWindow == nullptr) {
        LOGE("Cannot create session: native window is null");
        return false;
    }

    LOGI("Creating OpenXR session (window available)...");

    // Create session (requires native window)
    if (!CreateSession()) {
        LOGE("Failed to create OpenXR session");
        return false;
    }

    // Create reference space
    if (!CreateReferenceSpace()) {
        LOGE("Failed to create reference space");
        return false;
    }
    LOGI("Reference space created: %p", (void*)m_referenceSpace);

    // Create swapchains
    if (!CreateSwapchains()) {
        LOGE("Failed to create swapchains");
        return false;
    }

    // Create input manager
    m_inputManager = std::make_unique<InputManager>();
    if (!m_inputManager->Initialize(m_instance, m_session)) {
        LOGE("Failed to initialize input manager");
        return false;
    }

    // Create graphics renderer
    m_renderer = std::make_unique<GraphicsRenderer>();
    if (!m_renderer->Initialize()) {
        LOGE("Failed to initialize graphics renderer");
        return false;
    }

    m_initialized = true;
    LOGI("OpenXR session initialization successful!");
    return true;
}

void OpenXRControllerApp::SetNativeWindow(ANativeWindow* window) {
    LOGI("SetNativeWindow: %p -> %p", m_nativeWindow, window);

    // Handle window being destroyed
    if (window == nullptr) {
        m_nativeWindow = nullptr;
        m_pendingWindow = nullptr;
        LOGI("Window destroyed");
        return;
    }

    // Store window and mark as pending for initialization
    m_nativeWindow = window;
    m_pendingWindow = window;
    LOGI("Window stored, will initialize in main loop");
}

bool OpenXRControllerApp::InitializeWithWindow() {
    if (m_pendingWindow == nullptr) {
        return false;
    }

    LOGI("InitializeWithWindow called");

    // Initialize EGL if not already done
    if (eglGetCurrentContext() == EGL_NO_CONTEXT) {
        LOGI("Initializing EGL...");
        if (!InitializeEGL()) {
            LOGE("Failed to initialize EGL!");
            return false;
        }
    }

    // If we have a window and instance is ready, create session
    if (m_instance != XR_NULL_HANDLE && !m_sessionCreated) {
        LOGI("Window available, initializing session...");
        if (!InitializeSession()) {
            LOGE("Failed to initialize session!");
            return false;
        }
    }

    m_pendingWindow = nullptr;
    return true;
}

bool OpenXRControllerApp::InitializeEGL() {
    LOGI("Setting up EGL...");

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed");
        return false;
    }

    EGLint majorVersion, minorVersion;
    if (!eglInitialize(display, &majorVersion, &minorVersion)) {
        LOGE("eglInitialize failed");
        return false;
    }

    LOGI("EGL version: %d.%d", majorVersion, minorVersion);

    // Choose config
    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs)) {
        LOGE("eglChooseConfig failed");
        return false;
    }

    // Create context
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed");
        return false;
    }

    // Create surface with the native window
    EGLSurface surface = eglCreateWindowSurface(display, config, m_nativeWindow, nullptr);
    if (surface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        eglDestroyContext(display, context);
        return false;
    }

    // Make current
    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOGE("eglMakeCurrent failed");
        eglDestroySurface(display, surface);
        eglDestroyContext(display, context);
        return false;
    }

    LOGI("EGL initialized successfully: display=%p, context=%p, surface=%p",
         display, context, surface);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    return true;
}

void OpenXRControllerApp::Shutdown() {
    LOGI("Shutting down...");

    // Delete global reference
    if (m_globalActivityRef != nullptr && m_androidApp != nullptr) {
        JNIEnv* env = nullptr;
        JavaVM* vm = m_androidApp->activity->vm;
        if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
            env->DeleteGlobalRef(m_globalActivityRef);
            LOGI("Deleted global ref for Activity");
        }
        m_globalActivityRef = nullptr;
    }

    m_renderer.reset();
    m_inputManager.reset();

    // Destroy framebuffers and depth renderbuffers
    for (auto& eye : m_eyeViews) {
        if (!eye.framebuffers.empty()) {
            glDeleteFramebuffers(static_cast<GLsizei>(eye.framebuffers.size()),
                                 eye.framebuffers.data());
            eye.framebuffers.clear();
        }
        if (!eye.depthRenderbuffers.empty()) {
            glDeleteRenderbuffers(static_cast<GLsizei>(eye.depthRenderbuffers.size()),
                                  eye.depthRenderbuffers.data());
            eye.depthRenderbuffers.clear();
        }
        if (eye.swapchain != XR_NULL_HANDLE) {
            xrDestroySwapchain(eye.swapchain);
            eye.swapchain = XR_NULL_HANDLE;
        }
    }

    // Destroy reference space
    if (m_referenceSpace != XR_NULL_HANDLE) {
        xrDestroySpace(m_referenceSpace);
        m_referenceSpace = XR_NULL_HANDLE;
    }

    // Destroy session
    if (m_session != XR_NULL_HANDLE) {
        xrDestroySession(m_session);
        m_session = XR_NULL_HANDLE;
    }

    // Destroy instance
    if (m_instance != XR_NULL_HANDLE) {
        xrDestroyInstance(m_instance);
        m_instance = XR_NULL_HANDLE;
    }

    m_sessionCreated = false;
    m_initialized = false;

    LOGI("Shutdown complete");
}

bool OpenXRControllerApp::InitializeLoader() {
    LOGI("Initializing OpenXR loader...");

    // Get JNI environment for current thread
    JNIEnv* env = nullptr;
    JavaVM* vm = m_androidApp->activity->vm;
    jint attachResult = vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);

    if (attachResult == JNI_EDETACHED) {
        JavaVMAttachArgs attachArgs = {};
        attachArgs.version = JNI_VERSION_1_6;
        attachArgs.name = "OpenXR_Main";
        attachArgs.group = nullptr;

        attachResult = vm->AttachCurrentThread(&env, &attachArgs);
        if (attachResult != JNI_OK) {
            LOGE("Failed to attach thread to JVM: %d", attachResult);
            return false;
        }
        LOGI("Thread attached to JVM");
    } else if (attachResult != JNI_OK) {
        LOGE("GetEnv failed: %d", attachResult);
        return false;
    }

    // Create global reference for Activity to prevent GC
    m_globalActivityRef = env->NewGlobalRef(m_androidApp->activity->clazz);
    if (m_globalActivityRef == nullptr) {
        LOGE("Failed to create global ref for Activity");
        return false;
    }
    LOGI("Created global ref for Activity: %p", m_globalActivityRef);

    XrLoaderInitInfoAndroidKHR loaderInitInfo = {
        XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR,
        nullptr,
        vm,
        m_globalActivityRef  // Use global ref instead of local ref
    };

    PFN_xrInitializeLoaderKHR pfnInitializeLoader = nullptr;
    XrResult result = xrGetInstanceProcAddr(
        XR_NULL_HANDLE,
        "xrInitializeLoaderKHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&pfnInitializeLoader)
    );

    if (XR_SUCCEEDED(result) && pfnInitializeLoader != nullptr) {
        result = pfnInitializeLoader(reinterpret_cast<XrLoaderInitInfoBaseHeaderKHR*>(&loaderInitInfo));
        if (XR_FAILED(result)) {
            LOGE("xrInitializeLoaderKHR failed: %d", result);
            return false;
        }
        LOGI("Loader initialized successfully");
    } else {
        LOGI("xrInitializeLoaderKHR not found, assuming static loader");
    }

    return true;
}

bool OpenXRControllerApp::CreateInstance() {
    LOGI("Creating OpenXR instance...");

    // Required extensions for Android platform
    const char* extensions[] = {
        XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
        XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME
    };
    uint32_t extensionCount = sizeof(extensions) / sizeof(extensions[0]);

    // Android-specific instance create info (required by PICO runtime)
    XrInstanceCreateInfoAndroidKHR androidCreateInfo = {
        XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR,
        nullptr,
        m_androidApp->activity->vm,
        m_globalActivityRef  // Use global reference
    };

    XrApplicationInfo appInfo = {};
    strncpy(appInfo.applicationName, "OpenXR Controller Demo", XR_MAX_APPLICATION_NAME_SIZE - 1);
    appInfo.applicationVersion = 1;
    strncpy(appInfo.engineName, "OpenXRControllerDemo", XR_MAX_ENGINE_NAME_SIZE - 1);
    appInfo.engineVersion = 1;
    appInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrInstanceCreateInfo createInfo = {
        XR_TYPE_INSTANCE_CREATE_INFO,
        &androidCreateInfo,  // Chain Android-specific info
        0,
        appInfo,
        0,
        nullptr,
        extensionCount,
        extensions
    };

    XrResult result = xrCreateInstance(&createInfo, &m_instance);
    if (XR_FAILED(result)) {
        LOGE("xrCreateInstance failed: %d", result);
        return false;
    }

    m_instanceCreated = true;

    // Get graphics requirements function
    result = xrGetInstanceProcAddr(
        m_instance,
        "xrGetOpenGLESGraphicsRequirementsKHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLESGraphicsRequirementsKHR)
    );

    if (XR_FAILED(result) || pfnGetOpenGLESGraphicsRequirementsKHR == nullptr) {
        LOGE("Failed to get xrGetOpenGLESGraphicsRequirementsKHR");
        return false;
    }

    LOGI("OpenXR instance created successfully");
    return true;
}

bool OpenXRControllerApp::GetSystem() {
    LOGI("Getting OpenXR system...");

    XrSystemGetInfo systemInfo = {
        XR_TYPE_SYSTEM_GET_INFO,
        nullptr,
        XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY
    };

    XrResult result = xrGetSystem(m_instance, &systemInfo, &m_systemId);
    if (XR_FAILED(result)) {
        LOGE("xrGetSystem failed: %d", result);
        return false;
    }

    LOGI("OpenXR system ID: %lu", m_systemId);
    m_systemCreated = true;

    // Check graphics requirements
    XrGraphicsRequirementsOpenGLESKHR graphicsReq = {
        XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR
    };

    result = pfnGetOpenGLESGraphicsRequirementsKHR(m_instance, m_systemId, &graphicsReq);
    if (XR_FAILED(result)) {
        LOGE("xrGetOpenGLESGraphicsRequirementsKHR failed: %d", result);
        return false;
    }

    LOGI("OpenGL ES requirements: min %lu, max %lu",
         graphicsReq.minApiVersionSupported,
         graphicsReq.maxApiVersionSupported);

    // Query view configuration
    uint32_t viewConfigCount = 0;
    result = xrEnumerateViewConfigurations(m_instance, m_systemId, 0, &viewConfigCount, nullptr);
    if (XR_FAILED(result)) {
        LOGE("xrEnumerateViewConfigurations failed: %d", result);
        return false;
    }

    LOGI("Available view configurations: %u", viewConfigCount);

    // Get view configuration views for stereo rendering
    uint32_t viewCount = 0;
    result = xrEnumerateViewConfigurationViews(
        m_instance, m_systemId, m_viewConfigType, 0, &viewCount, nullptr);
    if (XR_FAILED(result)) {
        LOGE("xrEnumerateViewConfigurationViews failed: %d", result);
        return false;
    }

    m_viewConfigViews.resize(viewCount);
    for (auto& view : m_viewConfigViews) {
        view.type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
    }

    result = xrEnumerateViewConfigurationViews(
        m_instance, m_systemId, m_viewConfigType, viewCount, &viewCount, m_viewConfigViews.data());
    if (XR_FAILED(result)) {
        LOGE("xrEnumerateViewConfigurationViews (2) failed: %d", result);
        return false;
    }

    LOGI("View count: %u, Recommended resolution: %u x %u",
         viewCount,
         m_viewConfigViews[0].recommendedImageRectWidth,
         m_viewConfigViews[0].recommendedImageRectHeight);

    return true;
}

bool OpenXRControllerApp::CreateSession() {
    LOGI("Creating OpenXR session...");

    if (m_nativeWindow == nullptr) {
        LOGE("Native window not available");
        return false;
    }

    XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding = {
        XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
        nullptr,
        eglGetCurrentDisplay(),
        EGL_NO_CONFIG_KHR,
        eglGetCurrentContext()
    };

    XrSessionCreateInfo sessionCreateInfo = {
        XR_TYPE_SESSION_CREATE_INFO,
        &graphicsBinding,
        0,
        m_systemId
    };

    XrResult result = xrCreateSession(m_instance, &sessionCreateInfo, &m_session);
    if (XR_FAILED(result)) {
        LOGE("xrCreateSession failed: %d", result);
        return false;
    }

    m_sessionCreated = true;
    LOGI("OpenXR session created successfully");
    return true;
}

bool OpenXRControllerApp::CreateReferenceSpace() {
    LOGI("Creating reference space...");

    XrReferenceSpaceCreateInfo spaceCreateInfo = {
        XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
        nullptr,
        XR_REFERENCE_SPACE_TYPE_LOCAL,  // Use LOCAL instead of LOCAL_FLOOR for better compatibility
        {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}}
    };

    XrResult result = xrCreateReferenceSpace(m_session, &spaceCreateInfo, &m_referenceSpace);
    if (XR_FAILED(result)) {
        LOGE("xrCreateReferenceSpace failed: %d", result);
        return false;
    }

    LOGI("Reference space created");
    return true;
}

bool OpenXRControllerApp::CreateSwapchains() {
    LOGI("Creating swapchains...");

    for (int eye = 0; eye < 2; eye++) {
        auto& eyeView = m_eyeViews[eye];
        eyeView.width = m_viewConfigViews[eye].recommendedImageRectWidth;
        eyeView.height = m_viewConfigViews[eye].recommendedImageRectHeight;

        // Use GL_SRGB8_ALPHA8 for proper color space (standard for VR)
        XrSwapchainCreateInfo swapchainCreateInfo = {
            XR_TYPE_SWAPCHAIN_CREATE_INFO,
            nullptr,
            0,
            XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
            GL_SRGB8_ALPHA8,
            1,
            eyeView.width,
            eyeView.height,
            1,
            1,
            1
        };

        XrResult result = xrCreateSwapchain(m_session, &swapchainCreateInfo, &eyeView.swapchain);
        if (XR_FAILED(result)) {
            LOGE("xrCreateSwapchain failed for eye %d: %d", eye, result);
            return false;
        }

        // Get swapchain images
        uint32_t imageCount = 0;
        xrEnumerateSwapchainImages(eyeView.swapchain, 0, &imageCount, nullptr);

        eyeView.images.resize(imageCount);
        for (auto& image : eyeView.images) {
            image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
        }

        xrEnumerateSwapchainImages(
            eyeView.swapchain,
            imageCount,
            &imageCount,
            reinterpret_cast<XrSwapchainImageBaseHeader*>(eyeView.images.data()));

        LOGI("Eye %d swapchain created: %dx%d, %u images",
             eye, eyeView.width, eyeView.height, imageCount);
    }

    // Create framebuffers for each swapchain image
    if (!CreateFramebuffers()) {
        LOGE("Failed to create framebuffers");
        return false;
    }

    return true;
}

bool OpenXRControllerApp::CreateFramebuffers() {
    LOGI("Creating framebuffers for swapchain images...");

    for (int eye = 0; eye < 2; eye++) {
        auto& eyeView = m_eyeViews[eye];
        uint32_t imageCount = static_cast<uint32_t>(eyeView.images.size());

        // Create FBO and depth renderbuffer for each swapchain image
        eyeView.framebuffers.resize(imageCount);
        eyeView.depthRenderbuffers.resize(imageCount);
        glGenFramebuffers(imageCount, eyeView.framebuffers.data());
        glGenRenderbuffers(imageCount, eyeView.depthRenderbuffers.data());

        for (uint32_t i = 0; i < imageCount; i++) {
            // Setup depth renderbuffer for this image
            glBindRenderbuffer(GL_RENDERBUFFER, eyeView.depthRenderbuffers[i]);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                                  eyeView.width, eyeView.height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            // Setup framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, eyeView.framebuffers[i]);

            // Attach color (swapchain image)
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, eyeView.images[i].image, 0);

            // Attach depth (unique per image to avoid conflicts)
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER, eyeView.depthRenderbuffers[i]);

            // Check framebuffer completeness
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                LOGE("Framebuffer %d for eye %d incomplete: 0x%x", i, eye, status);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                return false;
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        LOGI("Eye %d: Created %u framebuffers with depth buffer", eye, imageCount);
    }

    LOGI("Framebuffers created successfully");
    return true;
}

void OpenXRControllerApp::PollEvents() {
    XrEventDataBuffer event = {XR_TYPE_EVENT_DATA_BUFFER};

    while (xrPollEvent(m_instance, &event) == XR_SUCCESS) {
        switch (event.type) {
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                auto* stateEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
                m_sessionState = stateEvent->state;

                LOGI("Session state changed to: %d", m_sessionState);

                switch (m_sessionState) {
                    case XR_SESSION_STATE_IDLE:
                        LOGI("Session state: IDLE");
                        break;
                    case XR_SESSION_STATE_READY:
                        LOGI("Session state: READY - beginning session...");
                        {
                            XrSessionBeginInfo beginInfo = {
                                XR_TYPE_SESSION_BEGIN_INFO,
                                nullptr,
                                m_viewConfigType
                            };
                            XrResult result = xrBeginSession(m_session, &beginInfo);
                            if (XR_FAILED(result)) {
                                LOGE("xrBeginSession failed: %d", result);
                            } else {
                                LOGI("xrBeginSession succeeded");
                                // PICO SDK sets session running to true immediately after xrBeginSession
                                // This allows the render loop to start calling xrWaitFrame
                                m_sessionRunning = true;
                                LOGI("Session marked as running (READY state)");
                            }
                        }
                        break;
                    case XR_SESSION_STATE_SYNCHRONIZED:
                        LOGI("Session state: SYNCHRONIZED (already running)");
                        break;
                    case XR_SESSION_STATE_VISIBLE:
                        LOGI("Session state: VISIBLE (already running)");
                        break;
                    case XR_SESSION_STATE_FOCUSED:
                        LOGI("Session state: FOCUSED (running and focused)");
                        break;
                    case XR_SESSION_STATE_STOPPING:
                        m_sessionRunning = false;
                        LOGI("Session state: STOPPING");
                        break;
                    case XR_SESSION_STATE_LOSS_PENDING:
                        LOGI("Session state: LOSS_PENDING");
                        break;
                    case XR_SESSION_STATE_EXITING:
                        m_shouldExit = true;
                        LOGI("Session state: EXITING");
                        break;
                    default:
                        LOGI("Session state: UNKNOWN (%d)", m_sessionState);
                        break;
                }
                break;
            }
            default:
                LOGI("Unhandled event type: %d", event.type);
                break;
        }

        event.type = XR_TYPE_EVENT_DATA_BUFFER;
    }
}

void OpenXRControllerApp::Update() {
    if (!m_initialized) {
        return;
    }

    PollEvents();

    if (m_sessionRunning && m_inputManager && m_referenceSpace != XR_NULL_HANDLE) {
        m_inputManager->Update(m_session, m_predictedDisplayTime, m_referenceSpace);

        // Update controller states
        for (int hand = 0; hand < 2; hand++) {
            m_controllers[hand] = m_inputManager->GetControllerState(hand);
            if (m_controllers[hand].active) {
                LogControllerState(hand, m_controllers[hand]);
            }
        }
    }

    static int updateCounter = 0;
    if (++updateCounter % 60 == 0) {
        LOGI("Update tick: initialized=%d, sessionRunning=%d", m_initialized, m_sessionRunning);
    }
}

void OpenXRControllerApp::RenderFrame() {
    if (!m_initialized || !m_sessionRunning) {
        return;
    }

    // Check reference space before rendering
    if (m_referenceSpace == XR_NULL_HANDLE) {
        LOGE("RenderFrame: reference space is null!");
        return;
    }

    LOGI("RenderFrame: initialized=%d, sessionRunning=%d, refSpace=%p", m_initialized, m_sessionRunning, (void*)m_referenceSpace);

    XrFrameWaitInfo waitInfo = {XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState = {XR_TYPE_FRAME_STATE};

    XrResult result = xrWaitFrame(m_session, &waitInfo, &frameState);
    if (XR_FAILED(result)) {
        LOGE("xrWaitFrame failed: %d (0x%X), skipping frame", result, result);
        // Even if xrWaitFrame fails, we should continue to next frame
        // to avoid blocking the main loop
        return;
    }

    m_predictedDisplayTime = frameState.predictedDisplayTime;

    static int waitFrameCount = 0;
    static int shouldRenderCount = 0;
    waitFrameCount++;
    if (frameState.shouldRender) {
        shouldRenderCount++;
    }
    if (waitFrameCount % 60 == 0) {
        LOGI("xrWaitFrame: total=%d, shouldRender=%d, ratio=%.1f%%",
             waitFrameCount, shouldRenderCount, 100.0f * shouldRenderCount / waitFrameCount);
    }

    LOGI("xrWaitFrame: shouldRender=%s", frameState.shouldRender ? "true" : "false");

    result = xrBeginFrame(m_session, nullptr);
    if (XR_FAILED(result)) {
        LOGE("xrBeginFrame failed: %d (0x%X), skipping frame", result, result);
        // Continue to next frame on error
        return;
    }

    // Get views
    XrViewLocateInfo viewLocateInfo = {
        XR_TYPE_VIEW_LOCATE_INFO,
        nullptr,
        m_viewConfigType,
        m_predictedDisplayTime,
        m_referenceSpace
    };

    XrViewState viewState = {XR_TYPE_VIEW_STATE};
    uint32_t viewCount = 0;
    std::vector<XrView> views(2);
    for (auto& view : views) {
        view.type = XR_TYPE_VIEW;
    }

    result = xrLocateViews(
        m_session,
        &viewLocateInfo,
        &viewState,
        views.size(),
        &viewCount,
        views.data()
    );

    if (XR_FAILED(result)) {
        LOGE("xrLocateViews failed: %d (0x%X), skipping frame", result, result);
        // Still need to call xrEndFrame if xrBeginFrame was called
        XrFrameEndInfo endInfo = {XR_TYPE_FRAME_END_INFO};
        endInfo.displayTime = m_predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endInfo.layerCount = 0;
        endInfo.layers = nullptr;
        xrEndFrame(m_session, &endInfo);
        return;
    }

    // Render each eye
    std::vector<XrCompositionLayerProjectionView> projectionViews;
    projectionViews.reserve(2);

    for (uint32_t eye = 0; eye < viewCount && eye < 2; eye++) {
        const auto& view = views[eye];
        auto& eyeView = m_eyeViews[eye];

        // Acquire swapchain image
        XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        uint32_t imageIndex = 0;
        XrResult acquireResult = xrAcquireSwapchainImage(eyeView.swapchain, &acquireInfo, &imageIndex);
        if (XR_FAILED(acquireResult)) {
            LOGE("xrAcquireSwapchainImage failed for eye %d: %d", eye, acquireResult);
            continue;
        }

        // Wait for swapchain image to be available (required by PICO runtime)
        XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        waitInfo.timeout = XR_INFINITE_DURATION;
        XrResult waitResult = xrWaitSwapchainImage(eyeView.swapchain, &waitInfo);
        if (XR_FAILED(waitResult)) {
            LOGE("xrWaitSwapchainImage failed for eye %d: %d", eye, waitResult);
            // Release the image and continue
            XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(eyeView.swapchain, &releaseInfo);
            continue;
        }

        // Bind framebuffer (not the texture directly)
        glBindFramebuffer(GL_FRAMEBUFFER, eyeView.framebuffers[imageIndex]);
        glViewport(0, 0, eyeView.width, eyeView.height);

        // Clear with dark gray to make objects visible
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Enable depth test
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // Render scene
        if (m_renderer) {
            m_renderer->Render(eye, view, eyeView.width, eyeView.height, m_controllers);
        }

        // Unbind framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Release swapchain image
        XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        XrResult releaseResult = xrReleaseSwapchainImage(eyeView.swapchain, &releaseInfo);
        if (XR_FAILED(releaseResult)) {
            LOGE("xrReleaseSwapchainImage failed for eye %d: %d", eye, releaseResult);
        }

        // Set up composition layer - only add if we successfully rendered
        XrCompositionLayerProjectionView projView = {};
        projView.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        projView.next = nullptr;
        projView.pose = view.pose;
        projView.fov = view.fov;
        projView.subImage.swapchain = eyeView.swapchain;
        projView.subImage.imageRect.offset.x = 0;
        projView.subImage.imageRect.offset.y = 0;
        projView.subImage.imageRect.extent.width = static_cast<int32_t>(eyeView.width);
        projView.subImage.imageRect.extent.height = static_cast<int32_t>(eyeView.height);
        projView.subImage.imageArrayIndex = 0;
        projectionViews.push_back(projView);
    }

    // Submit composition layer only if we have at least one view
    if (projectionViews.empty()) {
        LOGE("No projection views to submit!");
        XrFrameEndInfo endInfo = {XR_TYPE_FRAME_END_INFO};
        endInfo.displayTime = m_predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endInfo.layerCount = 0;
        endInfo.layers = nullptr;
        xrEndFrame(m_session, &endInfo);
        return;
    }

    XrCompositionLayerProjection projectionLayer = {
        XR_TYPE_COMPOSITION_LAYER_PROJECTION,
        nullptr,
        0,
        m_referenceSpace,
        static_cast<uint32_t>(projectionViews.size()),
        projectionViews.data()
    };

    const XrCompositionLayerBaseHeader* layers[] = {
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer)
    };

    XrFrameEndInfo endInfo = {XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = m_predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 1;
    endInfo.layers = layers;

    result = xrEndFrame(m_session, &endInfo);
    if (XR_FAILED(result)) {
        LOGE("xrEndFrame failed: %d (0x%X)", result, result);
    } else {
        static int successCount = 0;
        if (++successCount % 60 == 0) {
            LOGI("xrEndFrame succeeded, %d frames submitted", successCount);
        }
    }

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenXRControllerApp::LogControllerState(int hand, const ControllerState& state) {
    LOGI("Controller %d: active=%d, pos=(%.2f,%.2f,%.2f), trigger=%.2f, grip=%.2f",
         hand,
         state.active,
         state.aimPose.position.x,
         state.aimPose.position.y,
         state.aimPose.position.z,
         state.triggerValue,
         state.gripValue);
}
