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

#include "AndroidOpenXrProgram.h"
#include "AndroidPlatformPlugin.h"
#include "OpenGLESGraphicsPlugin.h"
#include "TruncatedCone.h"
#include "Cube.h"
#include "ExceptionHandlerProgram.h"

namespace PVRSampleFW {
    bool ActivityMainLoopContext::IsResumed() const {
        if (android_app_ != nullptr) {
            AndroidActivityState *state = reinterpret_cast<AndroidActivityState *>(android_app_->userData);
            return state->resumed;
        }
        PLOGE("ActivityMainLoopContext::IsResumed return false for android_app_ is nullptr");
        return false;
    }

    bool ActivityMainLoopContext::ShouldExitMainLoop() const {
        if (android_app_ != nullptr) {
            return android_app_->destroyRequested;
        }
        PLOGE("ActivityMainLoopContext::ShouldExitMainLoop return false for android_app_ is nullptr");
        return false;
    }

    void ActivityMainLoopContext::HandleOsEvents(bool firstFrame) {
        // Read all pending events.
        for (;;) {
            int events;
            struct android_poll_source *source;
            int firstWaitTime = firstFrame ? 10 : -1;
            // If the timeout is zero, returns immediately without blocking.
            // If the timeout is negative, waits indefinitely until an event appears.
            const int timeoutMilliseconds = (!IsResumed() && !activity_based_program_->IsSessionRunning() &&
                                             android_app_->destroyRequested == 0)
                                                    //                    ? -1
                                                    ? firstWaitTime
                                                    : 0;
            if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, reinterpret_cast<void **>(&source)) < 0) {
                break;
            }

            // Process this event.
            if (source != nullptr) {
                source->process(android_app_, source);
            }

            /// NOTES: if you want to use NDK after version 27, you have to replace the
            /// ALooper_pollAll function with the ALooper_pollOnce function.
        }
    }

    void AndroidOpenXrProgram::Initialize() {
        PLOGI("AndroidOpenXrProgram::Initialize");
        CHECK(android_app_ != nullptr);

        /// Initialize the Android environment.
        InitializeAndroidEnv(android_app_);

        /// Create and the configuration
        if (nullptr == app_config_)
            app_config_ = std::make_shared<Configurations>();

        UpdateAppConfig();

        /// update the user input handling from app config
        use_input_handling_ = app_config_->use_input_handling;

        /// Initialize the platform plugin.
        if (EqualsIgnoreCase(app_config_->platform_plugin, "Android")) {
            platform_plugin_ = std::make_shared<AndroidPlatformPlugin>(app_config_, android_app_->activity->vm,
                                                                       android_app_->activity->clazz);
        } else {
            PLOGE("AndroidOpenXrProgram::Initialize platform plugin is not supported");
            return;
        }

        /// Initialize the graphics plugin.
        if (EqualsIgnoreCase(app_config_->graphics_plugin, "OpenGLES")) {
            graphics_plugin_ = std::make_shared<OpenGLESGraphicsPlugin>(app_config_);
        } else if (EqualsIgnoreCase(app_config_->graphics_plugin, "Vulkan")) {
            // TODO:
            PLOGE("AndroidOpenXrProgram::Initialize graphics plugin is not supported");
            return;
        } else {
            PLOGE("AndroidOpenXrProgram::Initialize graphics plugin is not supported");
            return;
        }

        // Init openxr loader
        InitializeLoader(XR_LOADER_PLATFORM_ANDROID, reinterpret_cast<void *>(android_app_->activity->vm),
                         reinterpret_cast<void *>(android_app_->activity->clazz));

        extension_features_manager_->SetPlatformPlugin(platform_plugin_);
        extension_features_manager_->SetGraphicsPlugin(graphics_plugin_);

        // initialize  performance settings
        auto performance_setting_plugin = std::dynamic_pointer_cast<EXTPerformanceSettings>(
                extension_features_manager_->GetRegisterExtension(XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME));
        if (nullptr != performance_setting_plugin) {
            performance_setting_plugin->InitializePerformanceLevels(app_config_->perf_setting_cpu_level,
                                                                    app_config_->perf_setting_gpu_level);
        } else {
            PLOGE("AndroidOpenXrProgram::Initialize performance setting plugin is not supported");
        }
        // initialize  thread settings
        auto thread_setting_plugin = std::dynamic_pointer_cast<KHRAndroidThreadSetting>(
                extension_features_manager_->GetRegisterExtension(XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME));
        /// NOW: main thread and render thread are the same thread
        auto main_thread_tid = gettid();
        auto render_thread_tid = gettid();
        if (nullptr != thread_setting_plugin) {
            thread_setting_plugin->InitializeThreadsTid(main_thread_tid, render_thread_tid);
        } else {
            PLOGE("AndroidOpenXrProgram::Initialize thread setting plugin is not supported");
        }
        // display refresh rate
        auto display_refresh_rate_plugin = std::dynamic_pointer_cast<FBDisplayRefreshRates>(
                extension_features_manager_->GetRegisterExtension(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME));
        if (nullptr != display_refresh_rate_plugin) {
            display_refresh_rate_plugin->SetConfiguredRefreshRate(app_config_->parsed.targetrefreshrate);
        } else {
            PLOGE("AndroidOpenXrProgram::Initialize display refresh rate plugin is not supported");
        }

        // Init openxr instance
        InitializeInstance(GetApplicationName());
        // Init openxr system
        InitializeSystem();

        // update the configuration
        platform_plugin_->UpdateConfigurationsAtPlatform(app_config_);
        graphics_plugin_->UpdateConfigurationsAtGraphics(app_config_);
        GetConfigFromConfigurations(*app_config_);

        // Init graphics device
        graphics_plugin_->InitializeGraphicsDevice(GetXrInstance(), GetXrSystemId(), this);

        // Init openxr session
        InitializeSession();

        // Init openxr swapchain
        InitializeSwapchains();

        if (EqualsIgnoreCase(app_config_->graphics_plugin, "OpenGLES")) {
            GuiGLContext context;
            auto *graphicsBinding = reinterpret_cast<XrGraphicsBindingOpenGLESAndroidKHR *>(
                    const_cast<XrBaseInStructure *>(graphics_plugin_->GetGraphicsBinding()));
            context.display = graphicsBinding->display;
            context.context = graphicsBinding->context;
            ImGuiRenderer::GetInstance()->Initialize(&context);
        } else {
            PLOGE("AndroidOpenXrProgram::Initialize gui graphics plugin is not supported");
            return;
        }

        // app Init
        if (!(CustomizedAppPostInit())) {
            PLOGE("CustomizedAppPostInit() failed");
            return;
        }

        PLOGI("AndroidOpenXrProgram::Initialize success");
    }

    bool AndroidOpenXrProgram::CustomizedAppPostInit() {
        // TODO: add controllers and rays

        return true;
    }

    void AndroidOpenXrProgram::Shutdown() {
        PLOGI("AndroidOpenXrProgram::Shutdown");

        /// Shutdown the graphics device
        graphics_plugin_->ShutdownGraphicsDevice();

        /// OpenXR region
        // destroy openxr session
        DestroySession();
        // destroy openxr instance
        DestroyInstance();
    }

    int AndroidOpenXrProgram::PollEvents() {
        return 0;
    }

    void AndroidOpenXrProgram::Run(struct android_app *app) {
        PLOGI("AndroidOpenXrProgram::Run");
        try {
            // Init android app
            android_app_ = app;

            /// Initialize the program.
            Initialize();

            /// enter the main Loop
            Loop();

            /// Shutdown the program.
            Shutdown();
        } catch (const XrException &e) {
            PLOGE("XrException: %s", e.what());
            SwitchToExceptionHandlerProgram(e);
        } catch (const std::exception &e) {
            PLOGE("Exception: %s", e.what());
            exit(EXIT_FAILURE);
        } catch (...) {
            PLOGE("Unknown exception");
            exit(EXIT_FAILURE);
        }
    }

    void AndroidOpenXrProgram::AppHandleCmd(struct android_app *app, int32_t cmd) {
        AndroidActivityState *appState = reinterpret_cast<AndroidActivityState *>(app->userData);

        switch (cmd) {
        // There is no APP_CMD_CREATE. The ANativeActivity creates the
        // application thread from onCreate(). The application thread
        // then calls android_main().
        case APP_CMD_START: {
            PLOGI("    APP_CMD_START");
            PLOGI("onStart()");
            break;
        }
        case APP_CMD_RESUME: {
            PLOGI("onResume()");
            PLOGI("    APP_CMD_RESUME");
            appState->resumed = true;
            break;
        }
        case APP_CMD_PAUSE: {
            PLOGI("onPause()");
            PLOGI("    APP_CMD_PAUSE");
            appState->resumed = false;
            break;
        }
        case APP_CMD_STOP: {
            PLOGI("onStop()");
            PLOGI("    APP_CMD_STOP");
            break;
        }
        case APP_CMD_DESTROY: {
            PLOGI("onDestroy()");
            PLOGI("    APP_CMD_DESTROY");
            appState->native_window = NULL;
            break;
        }
        case APP_CMD_INIT_WINDOW: {
            PLOGI("surfaceCreated()");
            PLOGI("    APP_CMD_INIT_WINDOW");
            appState->native_window = app->window;
            break;
        }
        case APP_CMD_TERM_WINDOW: {
            PLOGI("surfaceDestroyed()");
            PLOGI("    APP_CMD_TERM_WINDOW");
            appState->native_window = NULL;
            break;
        }
        }
    }

    std::vector<uint8_t> AndroidOpenXrProgram::LoadFileFromAsset(const std::string &filename) const {
        // open asset
        if (nullptr == asset_manager_) {
            PLOGE("asset_manager_ is nullptr");
            return std::vector<uint8_t>();
        }
        AAsset *asset = AAssetManager_open(asset_manager_, filename.c_str(), AASSET_MODE_STREAMING);
        if (nullptr == asset) {
            PLOGE("AAssetManager_open failed");
            return std::vector<uint8_t>();
        }
        // get asset size
        off_t assetSize = AAsset_getLength(asset);
        if (assetSize < 0) {
            PLOGE("AAsset_getLength failed");
            AAsset_close(asset);
            return std::vector<uint8_t>();
        }
        // read asset
        std::vector<uint8_t> buffer(assetSize);
        size_t numBytesRead = AAsset_read(asset, buffer.data(), buffer.size());
        if (numBytesRead != buffer.size()) {
            PLOGE("AAsset_read failed");
            AAsset_close(asset);
            return std::vector<uint8_t>();
        }
        PLOGI("file:%s AAsset_read success, size:%d", filename.c_str(), buffer.size());
        AAsset_close(asset);
        return buffer;
    }

    bool AndroidOpenXrProgram::IsResumed() const {
        if (android_app_ != nullptr) {
            AndroidActivityState *state = reinterpret_cast<AndroidActivityState *>(android_app_->userData);
            return state->resumed;
        }
        PLOGE("ActivityMainLoopContext::IsResumed return false for android_app_ is nullptr");
        return false;
    }

    void AndroidOpenXrProgram::InitializeAndroidEnv(struct android_app *app) {
        PLOGI("InitializeAndroidEnv()");
        CHECK(main_loop_context_ == nullptr);

        JNIEnv *jniEnv = nullptr;
        app->activity->vm->AttachCurrentThread(&jniEnv, nullptr);

        app->userData = &app_state_;
        app->onAppCmd = AndroidOpenXrProgram::AppHandleCmd;

        main_loop_context_ =
                std::make_unique<ActivityMainLoopContext>(jniEnv, app->activity->clazz, app->activity->vm, app, this);

        // Initialize the other android objects
        internal_data_path_ = app->activity->internalDataPath;
        external_data_path_ = app->activity->externalDataPath;
        asset_manager_ = app->activity->assetManager;
    }

    void AndroidOpenXrProgram::UpdateAppConfig() {
        CHECK(app_config_ != nullptr);

        // now use configuration instead of system property
        /*char value[PROP_VALUE_MAX] = {};
        if (__system_property_get("debug.xr.graphicsPlugin", value) != 0) {
            app_config_->app_space_type = value;
        }

        if (__system_property_get("debug.xr.form_factor", value) != 0) {
            app_config_->form_factor = value;
        }

        if (__system_property_get("debug.xr.view_configuration", value) != 0) {
            app_config_->view_configuration = value;
        }

        if (__system_property_get("debug.xr.blendMode", value) != 0) {
            app_config_->environment_blend_mode = value;
        }*/

        // parse the configuration
        try {
            app_config_->ParseString();
            /*if (!CheckBlendMode(app_config_->parsed.environmentblendmode)) {
                PLOGE("Unsupported blend mode: %s, use Opaque instead",
                app_config_->environment_blend_mode.c_str());
                app_config_->environment_blend_mode = "Opaque";
                app_config_->parsed.environmentblendmode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
            }*/
        } catch (const std::exception &e) {
            PLOGE("Failed to parse the default configuration: %s", e.what());
            ShowSetAppConfigHelp();
            return;
        }
    }

    void AndroidOpenXrProgram::ShowSetAppConfigHelp() {
        ///TODO: need to add the Vukan-specific help message after the Vulkan plugin is implemented.
        /*PLOGI("adb shell setprop debug.xr.graphicsPlugin OpenGLES");
        PLOGI("adb shell setprop debug.xr.form_factor Hmd|Handheld");
        PLOGI("adb shell setprop debug.xr.view_configuration Stereo|Mono");
        PLOGI("adb shell setprop debug.xr.blendMode Opaque|Additive|AlphaBlend");*/

        PLOGI("use configuration to set app config");
    }

    void AndroidOpenXrProgram::Loop() {
        PLOGI("Loop()");
        bool requestRestart = false;
        bool exitRenderLoop = false;

        bool firstFrame = true;

        while (!main_loop_context_->ShouldExitMainLoop()) {
            if (firstFrame) {
                PLOGI("First frame, enter HandleOsEvents");
            }
            /// handle the events from the android activity_ main Loop context.
            main_loop_context_->HandleOsEvents(firstFrame);

            if (firstFrame) {
                PLOGI("First frame, enter openxr HandleXrEvents");
            }

            /// handle the events from the openxr wrapper.
            HandleXrEvents(&exitRenderLoop, &requestRestart);

            /// handle exit request
            if (exitRenderLoop) {
                PLOGI("ExitRenderLoop()");
                ANativeActivity_finish(android_app_->activity);
                continue;
            }

            if (!IsSessionRunning()) {
                // Throttle Loop since xrWaitFrame won't be called.
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                PLOGI("skip this frame for session is not running");
                continue;
            }

            if (firstFrame) {
                PLOGI("First frame, enter openxr DoFrame");
            }

            ImGuiRenderer::GetInstance()->TriggerSignal();

            DoFrame();

            // handle collision detection
            HandleCollisionDetection();

            if (firstFrame) {
                PLOGI("First frame, completed");
                firstFrame = false;
            }
        }

        PLOGI("Loop() exit");
    }

    void AndroidOpenXrProgram::HandleCollisionDetection() {
        /// now only collisions between rays from the controller and the gui are considered
        if (nullptr == collision_detector_) {
            collision_detector_ = SampleCollisionDetector::GetInstance();
        }

        for (size_t hand = 0; hand < Side::COUNT; hand++) {
            auto currentFrameIn = GetCurrentFrameIn();
            if (currentFrameIn.controller_actives[hand]) {
                bool bTrigger = currentFrameIn.controller_trigger_value[hand] > 0.8f;
                float distance = 100.0f;
                collision_detector_->DetectIntersection(&scenes_, currentFrameIn.controller_aim_poses[hand], &distance,
                                                        bTrigger, hand);

                // update ray
                UpdateRay(currentFrameIn.controller_aim_poses[hand], distance, hand);
            } else {
                // remove ray object if exists
                if (ray_obj_id_[hand] != -1) {
                    Scene &rayScene = scenes_.at(SAMPLE_SCENE_TYPE_RAY);
                    rayScene.RemoveObject(ray_obj_id_[hand]);
                    ray_obj_id_[hand] = -1;
                }
            }
        }
    }

    void AndroidOpenXrProgram::UpdateRay(const XrPosef &pose, const float &distance, const int &hand) {
        if (hand < 0 || hand > 1) {
            return;
        }
        const auto &sideId = ray_obj_id_[hand];
        Scene &rayScene = scenes_.at(SAMPLE_SCENE_TYPE_RAY);
        auto ray = rayScene.GetObject(sideId);
        XrVector3f handScale = {0.001f, 0.001f, distance};
        if (ray == nullptr) {
            int segments = 32;
            ray = std::make_shared<PVRSampleFW::TruncatedCone>(pose, handScale, segments, 85.0f * M_PI / 180.0f);
            rayScene.AddObject(ray);
            ray_obj_id_[hand] = ray->GetId();
        } else {
            ray->SetPose(pose);
            ray->SetScale(handScale);
        }
    }

    void AndroidOpenXrProgram::CustomizedExtensionAndFeaturesInit() {
        BasicOpenXrWrapper::CustomizedExtensionAndFeaturesInit();
        // setup non-plugin extensions
        std::vector<std::string> non_plugin_extension = {
                XR_BD_CONTROLLER_INTERACTION_EXTENSION_NAME,
        };
        non_plugin_extensions_.swap(non_plugin_extension);

        // setup your customized feature plugins here
        BasicOpenXrWrapper::CustomizedExtensionAndFeaturesInit();
        std::vector<std::string> extensions = {
                XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME,
                XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME,
                XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME,
        };
        extension_features_manager_->RegisterExtensionFeatures(extensions, this);
    }

    void AndroidOpenXrProgram::RenderFrame() {
        /// reset
        comp_layer_count_ = 0;
        memset(comp_layers_, 0, sizeof(PVRSampleFW::XrCompositionLayerUnion) * MAX_NUM_COMPOSITION_LAYERS);

        /// pre render frame
        if (!CustomizedPreRenderFrame()) {
            PLOGE("CustomizedPreRenderFrame failed");
            THROW_XR(XR_ERROR_RUNTIME_FAILURE, "CustomizedPreRenderFrame failed");
        }

        /// Render the scene.
        RenderScenes();

        if (!CustomizedRender()) {
            PLOGE("CustomizedRenderScene failed!");
        }

        /// post render frame
        if (!CustomizedPostRenderFrame()) {
            PLOGE("CustomizedPostRenderFrame failed");
            THROW_XR(XR_ERROR_RUNTIME_FAILURE, "CustomizedPostRenderFrame failed");
        }
    }

    void AndroidOpenXrProgram::RenderScenes() {
        // use composition layers to render a simple scene.
        XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
        projection_layer_views_.resize(config_views_.size());

        /// Render view to the appropriate part of the swapchain image.
        for (uint32_t eye = 0; eye < config_views_.size(); eye++) {
            const PVRSampleFW::Swapchain viewSwapchain = swapchains_[eye];
            XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            uint32_t swapchainImageIndex;
            CHECK_XRCMD(xrAcquireSwapchainImage(viewSwapchain.handle, &acquireInfo, &swapchainImageIndex));

            XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            waitInfo.timeout = XR_INFINITE_DURATION;
            CHECK_XRCMD(xrWaitSwapchainImage(viewSwapchain.handle, &waitInfo));

            projection_layer_views_[eye] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
            projection_layer_views_[eye].pose = current_frame_in_.views[eye].pose;
            projection_layer_views_[eye].fov = current_frame_in_.views[eye].fov;
            projection_layer_views_[eye].subImage.swapchain = viewSwapchain.handle;
            projection_layer_views_[eye].subImage.imageRect.offset = {0, 0};
            projection_layer_views_[eye].subImage.imageRect.extent = {viewSwapchain.width, viewSwapchain.height};

            const XrSwapchainImageBaseHeader *const swapchainImage =
                    swapchain_images_[viewSwapchain.handle][swapchainImageIndex];
            graphics_plugin_->RenderProjectionView(projection_layer_views_[eye], swapchainImage,
                                                   color_swapchain_format_, scenes_);

            XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            CHECK_XRCMD(xrReleaseSwapchainImage(viewSwapchain.handle, &releaseInfo));
        }
        // populate the projection layer.
        layer.space = app_space_;
        layer.layerFlags = xr_environment_blend_mode_ == XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND
                                   ? XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT |
                                             XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT
                                   : 0;
        layer.viewCount = static_cast<uint32_t>(projection_layer_views_.size());
        layer.views = projection_layer_views_.data();

        /// add to composition layers
        comp_layers_[comp_layer_count_++].projection = layer;
    }

    void AndroidOpenXrProgram::SwitchToExceptionHandlerProgram(const XrException &e) {
        PLOGI("SwitchToExceptionHandlerProgram");
        auto exception = e;
        auto program = std::make_shared<ExceptionHandlerProgram>(app_config_, &exception);
        program->Run(android_app_);
    }
}  // namespace PVRSampleFW
