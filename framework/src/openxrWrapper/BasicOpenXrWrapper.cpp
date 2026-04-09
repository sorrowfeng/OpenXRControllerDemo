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
#include "openxrWrapper/BasicOpenXrWrapper.h"
#include "util/CheckUtils.h"
#include "xr_linear.h"
#include "graphicsPlugin/IXrGraphicsPlugin.h"
#include "ExceptionUtils.h"

namespace PVRSampleFW {

    void BasicOpenXrWrapper::InitializeLoader(XrLoaderPlatformType loaderType, void* applicationVM,
                                              void* applicationContext) {
        PFN_xrInitializeLoaderKHR initializeLoader = nullptr;

        /// Initialize loader for the this platform.
        if (loaderType == XR_LOADER_PLATFORM_ANDROID) {
            if (XR_SUCCEEDED(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                                   reinterpret_cast<PFN_xrVoidFunction*>(&initializeLoader)))) {
                XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid = {XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
                loaderInitInfoAndroid.applicationVM = applicationVM;
                loaderInitInfoAndroid.applicationContext = applicationContext;
                initializeLoader((reinterpret_cast<XrLoaderInitInfoBaseHeaderKHR*>(&loaderInitInfoAndroid)));
            } else {
                PLOGE("Failed to get xrInitializeLoaderKHR");
            }
        }

        // Log the layers and extensions available on this system.
        GetLayersAndExtensions();

        /// init features
        // 1. create extension features and mgr
        if (nullptr == extension_features_manager_) {
            extension_features_manager_ = std::make_unique<ExtensionFeaturesManager>();
        }

        CustomizedExtensionAndFeaturesInit();

        /// extension features cb
        auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
        for (auto& feature : extensions) {
            feature->OnLoaderInit();
        }
    }

    bool BasicOpenXrWrapper::InitializeInstance(std::string applicationName) {
        /// get extensions
        std::vector<const char*> allExtensions;
        const std::vector<std::string> platformExtensions = platform_plugin_->GetInstanceExtensionsRequiredByPlatform();
        const std::vector<std::string> graphicsExtensions = graphics_plugin_->GetInstanceExtensionsRequiredByGraphics();
        // get featured extensions
        std::vector<std::string> featuredExtensions;
        if (nullptr != extension_features_manager_) {
            auto features = extension_features_manager_->GetAllRegisteredExtensions();
            for (auto& feature : features) {
                auto extArray = feature->GetRequiredExtensions();
                featuredExtensions.insert(featuredExtensions.end(), std::make_move_iterator(extArray.begin()),
                                          std::make_move_iterator(extArray.end()));
            }
        }
        enabled_extensions_.insert(enabled_extensions_.end(), std::make_move_iterator(platformExtensions.begin()),
                                   std::make_move_iterator(platformExtensions.end()));
        enabled_extensions_.insert(enabled_extensions_.end(), std::make_move_iterator(graphicsExtensions.begin()),
                                   std::make_move_iterator(graphicsExtensions.end()));
        enabled_extensions_.insert(enabled_extensions_.end(), non_plugin_extensions_.begin(),
                                   non_plugin_extensions_.end());
        enabled_extensions_.insert(enabled_extensions_.end(), std::make_move_iterator(featuredExtensions.begin()),
                                   std::make_move_iterator(featuredExtensions.end()));
        std::transform(enabled_extensions_.begin(), enabled_extensions_.end(), std::back_inserter(allExtensions),
                       [this](const std::string& ext) {
                           // check if extension is supported
                           if (this->IsExtensionSupported(ext.c_str())) {
                               return ext.c_str();
                           } else if (ext == XR_BD_CONTROLLER_INTERACTION_EXTENSION_NAME) {
                               //check if extension is XR_BD_CONTROLLER_INTERACTION_EXTENSION_NAME
                               return "";
                           } else {
                               throw ExtensionNotSupportedException(ext);
                               return "";
                           }
                       });

        XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        createInfo.next = platform_plugin_->GetInstanceCreateInfo();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(allExtensions.size());
        createInfo.enabledExtensionNames = allExtensions.data();

        // log the extensions
        for (const char* extension : allExtensions) {
            PLOGI("BasicOpenXrWrapper Extension: %s", extension);
        }

        strncpy(createInfo.applicationInfo.applicationName, applicationName.c_str(), XR_MAX_APPLICATION_NAME_SIZE);
        createInfo.applicationInfo.applicationVersion = 0;
        // Current version is 1.0.34
        createInfo.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 34);
        strncpy(createInfo.applicationInfo.engineName, "Pico XR Sample Framework", XR_MAX_ENGINE_NAME_SIZE);
        createInfo.applicationInfo.engineVersion = 0;

        // log the application info
        PLOGI("%s", Fmt("Application Name=%s Application Version=%d", createInfo.applicationInfo.applicationName,
                        createInfo.applicationInfo.applicationVersion)
                            .c_str());

        CHECK_XRCMD(xrCreateInstance(&createInfo, &xr_instance_));

        /// extension features cb
        auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
        for (auto& feature : extensions) {
            feature->OnInstanceCreate();
        }

        // Log the instance info.
        LogInstanceInfo();

        // Initialize the event handler for the instance.
        InitializeXrEventHandler();

        PLOGI("InitializeInstance successfully");

        return true;
    }

    void BasicOpenXrWrapper::GetLayersAndExtensions() {
        // Write out extension properties for a given layer.
        const auto logExtensions = [this](const char* layerName, int indent = 0, bool getAllExtensions = false) {
            uint32_t instanceExtensionCount;
            CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr));
            std::vector<XrExtensionProperties> extensions(instanceExtensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
            CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, static_cast<uint32_t>(extensions.size()),
                                                               &instanceExtensionCount, extensions.data()));

            const std::string indentStr(indent, ' ');
            PLOGV("%s", Fmt("%sAvailable Extensions: (%d)", indentStr.c_str(), instanceExtensionCount).c_str());
            for (const XrExtensionProperties& extension : extensions) {
                PLOGV("%s", Fmt("%s  Name=%s SpecVersion=%d", indentStr.c_str(), extension.extensionName,
                                extension.extensionVersion)
                                    .c_str());
            }

            // get all extensions
            if (getAllExtensions) {
                this->runtime_supported_extensions_.swap(extensions);
            }
        };

        // Log non-layer extensions (layerName==nullptr).
        logExtensions(nullptr, 0, true);

        // Log layers and any of their extensions.
        {
            uint32_t layerCount;
            CHECK_XRCMD(xrEnumerateApiLayerProperties(0, &layerCount, nullptr));
            std::vector<XrApiLayerProperties> layers(layerCount, {XR_TYPE_API_LAYER_PROPERTIES});
            CHECK_XRCMD(
                    xrEnumerateApiLayerProperties(static_cast<uint32_t>(layers.size()), &layerCount, layers.data()));

            PLOGV("%s", Fmt("Available Layers: (%d)", layerCount).c_str());
            for (const XrApiLayerProperties& layer : layers) {
                ///TODO: now we disable extensions from apiLayers
                logExtensions(layer.layerName, 4);
                PLOGV("%s", Fmt("  Name=%s SpecVersion=%s LayerVersion=%d Description=%s", layer.layerName,
                                GetXrVersionString(layer.specVersion).c_str(), layer.layerVersion, layer.description)
                                    .c_str());
            }
        }
    }

    void BasicOpenXrWrapper::LogInstanceInfo() {
        CHECK(xr_instance_ != XR_NULL_HANDLE);

        XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
        CHECK_XRCMD(xrGetInstanceProperties(xr_instance_, &instanceProperties));

        PLOGI("%s", Fmt("Instance RuntimeName=%s RuntimeVersion=%s", instanceProperties.runtimeName,
                        GetXrVersionString(instanceProperties.runtimeVersion).c_str())
                            .c_str());
    }

    bool BasicOpenXrWrapper::InitializeSystem() {
        PLOGI("InitializeSystem START");
        CHECK(xr_instance_ != XR_NULL_HANDLE);
        CHECK(xr_system_id_ == XR_NULL_SYSTEM_ID);
        CHECK(graphics_plugin_ != nullptr);

        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = xr_form_factor_;
        CHECK_XRCMD(xrGetSystem(xr_instance_, &systemInfo, &xr_system_id_));

        /// extension features cb
        auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
        for (auto& feature : extensions) {
            feature->OnSystemGet(&system_properties_);
        }

        GetSystemProperties();

        PLOGI("%s", Fmt("Using system %d for form factor %s", xr_system_id_, to_string(xr_form_factor_)).c_str());
        CHECK(xr_instance_ != XR_NULL_HANDLE);
        CHECK(xr_system_id_ != XR_NULL_SYSTEM_ID);

        // log view configuration
        LogViewConfigurations();

        PLOGI("InitializeSystem successfully");
        return true;
    }

    void BasicOpenXrWrapper::LogViewConfigurations() {
        CHECK(xr_instance_ != XR_NULL_HANDLE);
        CHECK(xr_system_id_ != XR_NULL_SYSTEM_ID);

        uint32_t viewConfigTypeCount;
        CHECK_XRCMD(xrEnumerateViewConfigurations(xr_instance_, xr_system_id_, 0, &viewConfigTypeCount, nullptr));
        std::vector<XrViewConfigurationType> viewConfigTypes(viewConfigTypeCount);
        CHECK_XRCMD(xrEnumerateViewConfigurations(xr_instance_, xr_system_id_, viewConfigTypeCount,
                                                  &viewConfigTypeCount, viewConfigTypes.data()));
        CHECK(static_cast<uint32_t>(viewConfigTypes.size()) == viewConfigTypeCount);

        PLOGI("%s", Fmt("Available View Configuration Types: (%d)", viewConfigTypeCount).c_str());
        for (XrViewConfigurationType viewConfigType : viewConfigTypes) {
            PLOGV("%s", Fmt("  View Configuration Type: %s %s", to_string(viewConfigType),
                            viewConfigType == config_view_config_type_ ? "(Selected)" : "")
                                .c_str());

            XrViewConfigurationProperties viewConfigProperties{XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
            CHECK_XRCMD(xrGetViewConfigurationProperties(xr_instance_, xr_system_id_, viewConfigType,
                                                         &viewConfigProperties));

            PLOGV("%s", Fmt("  View configuration FovMutable=%s",
                            viewConfigProperties.fovMutable == XR_TRUE ? "True" : "False")
                                .c_str());

            uint32_t viewCount;
            CHECK_XRCMD(xrEnumerateViewConfigurationViews(xr_instance_, xr_system_id_, viewConfigType, 0, &viewCount,
                                                          nullptr));
            if (viewCount > 0) {
                std::vector<XrViewConfigurationView> views(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
                CHECK_XRCMD(xrEnumerateViewConfigurationViews(xr_instance_, xr_system_id_, viewConfigType, viewCount,
                                                              &viewCount, views.data()));

                for (uint32_t i = 0; i < views.size(); i++) {
                    const XrViewConfigurationView& view = views[i];
                    PLOGV("%s", Fmt("    View [%d]: Recommended Width=%d Height=%d SampleCount=%d", i,
                                    view.recommendedImageRectWidth, view.recommendedImageRectHeight,
                                    view.recommendedSwapchainSampleCount)
                                        .c_str());
                    PLOGV("%s", Fmt("    View [%d]:     Maximum Width=%d Height=%d SampleCount=%d", i,
                                    view.maxImageRectWidth, view.maxImageRectHeight, view.maxSwapchainSampleCount)
                                        .c_str());
                }
            } else {
                PLOGE("Empty view configuration type");
            }

            LogEnvironmentBlendMode(viewConfigType);
        }

        PLOGI("LogViewConfigurations successfully");
    }

    void BasicOpenXrWrapper::LogEnvironmentBlendMode(XrViewConfigurationType type) {
        CHECK(xr_instance_ != XR_NULL_HANDLE);
        CHECK(xr_system_id_ != 0);

        uint32_t count;
        CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(xr_instance_, xr_system_id_, type, 0, &count, nullptr));
        CHECK(count > 0);

        PLOGI("Available Environment Blend Mode count : (%d)", count);
        std::vector<XrEnvironmentBlendMode> blendModes(count);
        CHECK_XRCMD(
                xrEnumerateEnvironmentBlendModes(xr_instance_, xr_system_id_, type, count, &count, blendModes.data()));

        bool blendModeFound = false;
        for (XrEnvironmentBlendMode mode : blendModes) {
            const bool blendModeMatch = (mode == xr_environment_blend_mode_);
            PLOGI("Environment Blend Mode (%s) : %s", to_string(mode), blendModeMatch ? "(Selected)" : "");
            blendModeFound |= blendModeMatch;
        }
        CHECK(blendModeFound);

        PLOGI("LogEnvironmentBlendMode successfully");
    }

    XrReferenceSpaceCreateInfo BasicOpenXrWrapper::CustomizedGetAppSpaceCreateInfo() {
        XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        XrPosef pose = {};
        XrPosef_CreateIdentity(&pose);
        referenceSpaceCreateInfo.poseInReferenceSpace = pose;
        referenceSpaceCreateInfo.referenceSpaceType = app_space_type_;
        return referenceSpaceCreateInfo;
    }

    XrActionSet BasicOpenXrWrapper::CreateActionSet(uint32_t priority, const char* name, const char* localizedName) {
        CHECK(xr_instance_ != XR_NULL_HANDLE);
        XrActionSetCreateInfo asci = {XR_TYPE_ACTION_SET_CREATE_INFO};
        asci.priority = priority;
        strcpy_s(asci.actionSetName, name);
        strcpy_s(asci.localizedActionSetName, localizedName);
        XrActionSet actionSet = XR_NULL_HANDLE;
        CHECK_XRCMD(xrCreateActionSet(xr_instance_, &asci, &actionSet));
        return actionSet;
    }

    XrAction BasicOpenXrWrapper::CreateAction(XrActionSet actionSet, XrActionType type, const char* actionName,
                                              const char* localizedName, int countSubactionPaths,
                                              XrPath* subactionPaths) {
        PLOGV("CreateAction %s, %d", actionName, countSubactionPaths);

        CHECK(actionSet != XR_NULL_HANDLE);
        XrActionCreateInfo actionCreateInfo = {XR_TYPE_ACTION_CREATE_INFO};
        actionCreateInfo.actionType = type;
        if (countSubactionPaths > 0) {
            actionCreateInfo.countSubactionPaths = countSubactionPaths;
            actionCreateInfo.subactionPaths = subactionPaths;
        }
        strcpy_s(actionCreateInfo.actionName, actionName);
        strcpy_s(actionCreateInfo.localizedActionName, localizedName ? localizedName : actionName);
        XrAction action = XR_NULL_HANDLE;
        CHECK_XRCMD(xrCreateAction(actionSet, &actionCreateInfo, &action));
        return action;
    }

    XrActionSuggestedBinding BasicOpenXrWrapper::ActionSuggestedBinding(XrAction action, XrPath bindingPath) {
        XrActionSuggestedBinding asb;
        asb.action = action;
        asb.binding = bindingPath;
        return asb;
    }

    void BasicOpenXrWrapper::SuggestInteractionProfileBindings(
            const std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> allSuggestedBindings) {
        // Best practice is for apps to suggest bindings for *ALL* interaction profiles
        // that the app supports. Loop over all interaction profiles we support and suggest
        // bindings:
        for (auto& [interactionProfilePath, bindings] : allSuggestedBindings) {
            XrInteractionProfileSuggestedBinding suggestedBindings = {XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile = interactionProfilePath;
            suggestedBindings.suggestedBindings = (const XrActionSuggestedBinding*)bindings.data();
            suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());

            CHECK_XRCMD(xrSuggestInteractionProfileBindings(xr_instance_, &suggestedBindings));
        }
    }

    XrSpace BasicOpenXrWrapper::CreateActionSpace(XrAction poseAction, XrPath subactionPath) {
        XrActionSpaceCreateInfo asci = {XR_TYPE_ACTION_SPACE_CREATE_INFO};
        asci.action = poseAction;
        asci.poseInActionSpace.orientation.w = 1.0f;
        asci.subactionPath = subactionPath;
        XrSpace actionSpace = XR_NULL_HANDLE;
        CHECK_XRCMD(xrCreateActionSpace(xr_session_, &asci, &actionSpace));
        return actionSpace;
    }

    bool BasicOpenXrWrapper::InitializeSession() {
        CHECK(xr_instance_ != XR_NULL_HANDLE);
        CHECK(xr_session_ == XR_NULL_HANDLE);
        CHECK(graphics_plugin_ != nullptr);

        // Create a session
        {
            PLOGI("Creating session");
            XrSessionCreateInfo createInfo{XR_TYPE_SESSION_CREATE_INFO};
            createInfo.next = graphics_plugin_->GetGraphicsBinding();
            createInfo.systemId = xr_system_id_;
            CHECK_XRCMD(xrCreateSession(xr_instance_, &createInfo, &xr_session_));
        }

        /// extension features cb
        auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
        for (auto& feature : extensions) {
            feature->OnSessionCreate();
        }

        LogReferenceSpaces();

        InitializeActions();

        // Customized input handler
        if (!CustomizedXrInputHandlerSetup()) {
            PLOGE("CustomizedXrInputHandlerSetup failed");
            return false;
        }

        // customized session init
        if (!CustomizedSessionInit()) {
            PLOGE("CustomizedSessionInit failed");
            return false;
        }

        // create app reference space
        {
            XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = CustomizedGetAppSpaceCreateInfo();
            CHECK_XRCMD(xrCreateReferenceSpace(xr_session_, &referenceSpaceCreateInfo, &app_space_));
        }

        // create head view space
        {
            XrReferenceSpaceCreateInfo spaceCreateInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
            spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
            spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
            CHECK_XRCMD(xrCreateReferenceSpace(xr_session_, &spaceCreateInfo, &head_view_space_));
        }
        return true;
    }

    void BasicOpenXrWrapper::LogReferenceSpaces() {
        CHECK(xr_session_ != XR_NULL_HANDLE);

        uint32_t spaceCount;
        CHECK_XRCMD(xrEnumerateReferenceSpaces(xr_session_, 0, &spaceCount, nullptr));
        std::vector<XrReferenceSpaceType> spaces(spaceCount);
        CHECK_XRCMD(xrEnumerateReferenceSpaces(xr_session_, spaceCount, &spaceCount, spaces.data()));

        PLOGI("Available reference spaces: %d", spaceCount);
        for (XrReferenceSpaceType space : spaces) {
            PLOGV("Available reference spaces:  Name: %s", to_string(space));
        }
    }

    void BasicOpenXrWrapper::InitializeViewConfiguration() {
        CHECK(config_views_.empty());

        // Query and cache view configuration views.
        uint32_t viewCount;
        CHECK_XRCMD(xrEnumerateViewConfigurationViews(xr_instance_, xr_system_id_, config_view_config_type_, 0,
                                                      &viewCount, nullptr));
        config_views_.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
        CHECK_XRCMD(xrEnumerateViewConfigurationViews(xr_instance_, xr_system_id_, config_view_config_type_, viewCount,
                                                      &viewCount, config_views_.data()));

        // Create and cache view buffer for xrLocateViews later.
        projection_views_cache_.resize(viewCount, {XR_TYPE_VIEW});
    }

    bool BasicOpenXrWrapper::InitializeSwapchains() {
        PLOGI("Initializing swapchains_");
        CHECK(xr_session_ != XR_NULL_HANDLE);

        // Note: No other view configurations exist at the time this code was written. If this
        // condition is not met, the project will need to be audited to see how support should be
        // added.
        CHECK_MSG(config_view_config_type_ == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                  "Unsupported view configuration type");

        InitializeViewConfiguration();

        // Create the swapchain and get the images.
        auto viewCount = static_cast<uint32_t>(config_views_.size());
        if (viewCount > 0) {
            // Select a swapchain format.
            uint32_t swapchainFormatCount;
            CHECK_XRCMD(xrEnumerateSwapchainFormats(xr_session_, 0, &swapchainFormatCount, nullptr));
            std::vector<int64_t> swapchainFormats(swapchainFormatCount);
            CHECK_XRCMD(xrEnumerateSwapchainFormats(xr_session_, static_cast<uint32_t>(swapchainFormats.size()),
                                                    &swapchainFormatCount, swapchainFormats.data()));
            CHECK(swapchainFormatCount == swapchainFormats.size());

            // todo, select color swapchain format
            color_swapchain_format_ = graphics_plugin_->SelectColorSwapchainFormat(swapchainFormats);

            // Print swapchain formats and the selected one.
            {
                std::string swapchainFormatsString;
                for (int64_t format : swapchainFormats) {
                    const bool selected = format == color_swapchain_format_;
                    swapchainFormatsString += " ";
                    if (selected) {
                        swapchainFormatsString += "[";
                    }
                    swapchainFormatsString += std::to_string(format);
                    if (selected) {
                        swapchainFormatsString += "]";
                    }
                }
                PLOGV("Swapchain Formats: %s", swapchainFormatsString.c_str());
            }

            // Create a swapchain for each view.
            for (uint32_t i = 0; i < viewCount; i++) {
                const XrViewConfigurationView& vp = config_views_[i];
                PLOGI("Creating swapchain for view %d with dimensions Width=%d Height=%d SampleCount=%d", i,
                      vp.recommendedImageRectWidth, vp.recommendedImageRectHeight, vp.recommendedSwapchainSampleCount);

                // Create the swapchain.
                XrSwapchainCreateInfo swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
                swapchainCreateInfo.arraySize = 1;
                swapchainCreateInfo.format = color_swapchain_format_;
                swapchainCreateInfo.width = vp.recommendedImageRectWidth;
                swapchainCreateInfo.height = vp.recommendedImageRectHeight;
                swapchainCreateInfo.mipCount = 1;
                swapchainCreateInfo.faceCount = 1;
                swapchainCreateInfo.sampleCount = graphics_plugin_->GetSupportedSwapchainSampleCount(vp);
                swapchainCreateInfo.usageFlags =
                        XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
                Swapchain swapchain;
                swapchain.width = swapchainCreateInfo.width;
                swapchain.height = swapchainCreateInfo.height;
                CHECK_XRCMD(xrCreateSwapchain(xr_session_, &swapchainCreateInfo, &swapchain.handle));

                swapchains_.push_back(swapchain);

                PLOGI("Swapchain created successfully with swapchains_ count: %d", swapchains_.size());

                uint32_t imageCount;
                CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr));
                // XXX This should really just return XrSwapchainImageBaseHeader*
                std::vector<XrSwapchainImageBaseHeader*> swapchainImgs =
                        graphics_plugin_->AllocateSwapchainImageStructs(imageCount, swapchainCreateInfo);
                CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, imageCount, &imageCount, swapchainImgs[0]));

                swapchain_images_.insert(std::make_pair(swapchain.handle, std::move(swapchainImgs)));
            }
        }

        /// extension features cb
        auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
        for (auto& feature : extensions) {
            feature->OnSwapchainsInit();
        }

        // customized swapchain init
        if (!CustomizedSwapchainsInit()) {
            PLOGE("CustomizedSwapchainInit failed");
            return false;
        }

        return true;
    }

    void BasicOpenXrWrapper::InitializeXrEventHandler() {
        // Register base event handlers.
        XrEventHandler eventHandler[4];
        eventHandler[0].event_type = XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING;
        eventHandler[0].handler = [](void* userData /*NULL*/, const XrEventDataBaseHeader* eventData,
                                     bool* exitRenderLoop, bool* requestRestart) {
            const auto& instanceLossPending = *reinterpret_cast<const XrEventDataInstanceLossPending*>(eventData);
            PLOGW("XrEventDataInstanceLossPending by %lld", instanceLossPending.lossTime);
            *exitRenderLoop = true;
            *requestRestart = true;
        };
        registered_event_handlers_.push_back(eventHandler[0]);
        eventHandler[1].event_type = XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED;
        eventHandler[1].handler = [](void* userData /*NULL*/, const XrEventDataBaseHeader* eventData,
                                     bool* exitRenderLoop, bool* requestRestart) {
            const auto& sessionStateChanged = *reinterpret_cast<const XrEventDataSessionStateChanged*>(eventData);
            PLOGI("XrEventDataSessionStateChanged %s", to_string(sessionStateChanged.state));
            BasicOpenXrWrapper* app = reinterpret_cast<BasicOpenXrWrapper*>(userData);
            app->HandleSessionStateChangeEvent(userData, eventData, exitRenderLoop, requestRestart);
        };
        registered_event_handlers_.push_back(eventHandler[1]);
        eventHandler[2].event_type = XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING;
        eventHandler[2].handler = [](void* userData /*NULL*/, const XrEventDataBaseHeader* eventData,
                                     bool* exitRenderLoop, bool* requestRestart) {
            const auto& referenceSpaceChangePending =
                    *reinterpret_cast<const XrEventDataReferenceSpaceChangePending*>(eventData);
            PLOGI("XrEventDataReferenceSpaceChangePending %s",
                  to_string(referenceSpaceChangePending.referenceSpaceType));
        };
        registered_event_handlers_.push_back(eventHandler[2]);
        eventHandler[3].event_type = XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED;
        eventHandler[3].handler = [](void* userData /*NULL*/, const XrEventDataBaseHeader* eventData,
                                     bool* exitRenderLoop, bool* requestRestart) {
            PLOGI("XrEventDataInteractionProfileChanged occur");
            BasicOpenXrWrapper* app = reinterpret_cast<BasicOpenXrWrapper*>(userData);
            app->HandleInteractionProfileChangedEvent(userData, eventData, exitRenderLoop, requestRestart);
        };
        registered_event_handlers_.push_back(eventHandler[3]);

        /// extension features cb
        auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
        for (auto& feature : extensions) {
            feature->OnEventHandlerSetup();
        }

        // customized event handler init
        if (!CustomizedXrEventHandlerSetup()) {
            PLOGE("CustomizedEventHandlerInit failed");
            return;
        }

        PLOGI("InitializeXrEventHandler success");
    }

    void BasicOpenXrWrapper::LogActionSourceName(XrAction action, const std::string& actionName) const {
        CHECK(xr_session_ != XR_NULL_HANDLE);

        XrBoundSourcesForActionEnumerateInfo getInfo = {XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO};
        getInfo.action = action;
        uint32_t pathCount = 0;
        CHECK_XRCMD(xrEnumerateBoundSourcesForAction(xr_session_, &getInfo, 0, &pathCount, nullptr));
        std::vector<XrPath> paths(pathCount);
        CHECK_XRCMD(xrEnumerateBoundSourcesForAction(xr_session_, &getInfo, static_cast<uint32_t>(paths.size()),
                                                     &pathCount, paths.data()));

        std::string sourceName;
        for (uint32_t i = 0; i < pathCount; ++i) {
            constexpr XrInputSourceLocalizedNameFlags all = XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT |
                                                            XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT |
                                                            XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

            XrInputSourceLocalizedNameGetInfo nameInfo = {XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO};
            nameInfo.sourcePath = paths[i];
            nameInfo.whichComponents = all;

            uint32_t size = 0;
            CHECK_XRCMD(xrGetInputSourceLocalizedName(xr_session_, &nameInfo, 0, &size, nullptr));
            if (size < 1) {
                continue;
            }
            std::vector<char> grabSource(size);
            CHECK_XRCMD(xrGetInputSourceLocalizedName(xr_session_, &nameInfo, static_cast<uint32_t>(grabSource.size()),
                                                      &size, grabSource.data()));
            if (!sourceName.empty()) {
                sourceName += " and ";
            }
            sourceName += "'";
            sourceName += std::string(grabSource.data(), size - 1);
            sourceName += "'";
        }

        PLOGI("%s action is bound to %s", actionName.c_str(), ((!sourceName.empty()) ? sourceName.c_str() : "nothing"));
    }

    void BasicOpenXrWrapper::HandleInteractionProfileChangedEvent(void* userData,
                                                                  const XrEventDataBaseHeader* eventData,
                                                                  bool* exitRenderLoop, bool* requestRestart) {
        const auto& interactionProfileChanged =
                *reinterpret_cast<const XrEventDataInteractionProfileChanged*>(eventData);
        PLOGI("XrEventDataInteractionProfileChanged occur");
        // just log some actions of input, you can design your own logic
        LogActionSourceName(input_.grab_action, "Grab");
        LogActionSourceName(input_.quit_action, "Quit");
        LogActionSourceName(input_.grip_pose_action, "Pose");
        LogActionSourceName(input_.vibrate_action, "Vibrate");
    }

    bool BasicOpenXrWrapper::GetConfigFromConfigurations(const Configurations& configurations) {
        config_view_config_type_ = configurations.parsed.viewconfigtype;
        xr_environment_blend_mode_ = configurations.parsed.environmentblendmode;
        xr_form_factor_ = configurations.parsed.formfactor;
        target_display_refresh_rate = configurations.parsed.targetrefreshrate;
        // update app space type
        if (EqualsIgnoreCase(configurations.app_space_type, "Local")) {
            app_space_type_ = XR_REFERENCE_SPACE_TYPE_LOCAL;
        } else if (EqualsIgnoreCase(configurations.app_space_type, "Stage")) {
            app_space_type_ = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(configurations.app_space_type, "LocalFloor")) {
            app_space_type_ = XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR;
            //        } else if (EqualsIgnoreCase(configurations.AppSpaceType, "UnboundedMsft")) {
            //            app_space_type_ = XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT;
            //        } else if (EqualsIgnoreCase(configurations.AppSpaceType, "CombinedEyeVarjo")) {
            //            app_space_type_ = XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO;
            //        } else if (EqualsIgnoreCase(configurations.AppSpaceType, "LocalizationMapML")) {
            //            app_space_type_ = XR_REFERENCE_SPACE_TYPE_LOCALIZATION_MAP_ML;
        } else if (EqualsIgnoreCase(configurations.app_space_type, "LocalFloorExt")) {
            app_space_type_ = XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT;
        } else {
            PLOGE("Invalid AppSpaceType: %s", configurations.app_space_type.c_str());
            return false;
        }
        return true;
    }

    void BasicOpenXrWrapper::HandleXrEvents(bool* exitRenderLoop, bool* requestRestart) {
        *exitRenderLoop = *requestRestart = false;

        // Process all pending messages.
        while (const XrEventDataBaseHeader* event = TryReadNextEvent()) {
            // Traverse the eventHandler set and process the events that meet the conditions
            for (const XrEventHandler& eventHandler : registered_event_handlers_) {
                if (event->type == eventHandler.event_type) {
                    eventHandler.handler(this, event, exitRenderLoop, requestRestart);
                }
            }
        }
    }

    const XrEventDataBaseHeader* BasicOpenXrWrapper::TryReadNextEvent() {
        // It is sufficient to clear the just the XrEventDataBuffer header to
        // XR_TYPE_EVENT_DATA_BUFFER
        XrEventDataBaseHeader* baseHeader = reinterpret_cast<XrEventDataBaseHeader*>(&event_data_buffer_);
        *baseHeader = {XR_TYPE_EVENT_DATA_BUFFER};
        const XrResult xr = xrPollEvent(xr_instance_, &event_data_buffer_);
        if (xr == XR_SUCCESS) {
            if (baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
                const XrEventDataEventsLost* const eventsLost =
                        reinterpret_cast<const XrEventDataEventsLost*>(baseHeader);
                PLOGW("%d events lost", eventsLost->lostEventCount);
            }

            return baseHeader;
        }
        if (xr == XR_EVENT_UNAVAILABLE) {
            return nullptr;
        }
        THROW_XR(xr, "xrPollEvent");
    }

    void BasicOpenXrWrapper::HandleSessionStateChangeEvent(void* userData, const XrEventDataBaseHeader* eventData,
                                                           bool* exitRenderLoop, bool* requestRestart) {
        const XrSessionState oldState = xr_session_state_;
        const XrEventDataSessionStateChanged* const sessionStateChanged =
                reinterpret_cast<const XrEventDataSessionStateChanged*>(eventData);

        // Update the session state.
        if (!CustomizedSessionStateChange(sessionStateChanged->state)) {
            PLOGE("CustomizedSessionStateChange failed");
            THROW_XR(XR_ERROR_RUNTIME_FAILURE, "CustomizedSessionStateChange failed");
        }

        PLOGI("XrEventDataSessionStateChanged from oldState: %s to state: %s", to_string(oldState),
              to_string(sessionStateChanged->state));

        if ((sessionStateChanged->session != XR_NULL_HANDLE) && (sessionStateChanged->session != xr_session_)) {
            PLOGE("XrEventDataSessionStateChanged for unknown session");
            return;
        }

        // Process the session state change.
        switch (sessionStateChanged->state) {
        case XR_SESSION_STATE_READY: {
            CHECK(xr_session_ != XR_NULL_HANDLE);
            XrSessionBeginInfo sessionBeginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
            sessionBeginInfo.primaryViewConfigurationType = config_view_config_type_;

            XrResult result;
            CHECK_XRCMD(xrBeginSession(xr_session_, &sessionBeginInfo));
            is_session_running_ = true;
            PLOGI("HandleSessionStateChangeEvent session Running");

            /// extension features cb
            auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
            for (auto& feature : extensions) {
                feature->OnSessionBegin();
            }
        } break;
        case XR_SESSION_STATE_STOPPING:
            // Stop the session.
            if (is_session_running_) {
                CHECK_XRCMD(xrEndSession(xr_session_));
                is_session_running_ = false;
                PLOGI("HandleSessionStateChangeEvent session Stopping");

                /// extension features cb
                auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
                for (auto& feature : extensions) {
                    feature->OnSessionEnd();
                }
            }
            break;
        case XR_SESSION_STATE_EXITING:
            // Exit the application.
            *exitRenderLoop = true;
            // Do not attempt to restart because user closed this session.
            *requestRestart = false;
            break;
        case XR_SESSION_STATE_LOSS_PENDING: {
            *exitRenderLoop = true;
            // Poll for a new instance.
            *requestRestart = true;
            break;
        }
        default:
            break;
        }
    }

    bool BasicOpenXrWrapper::IsSessionRunning() const {
        return is_session_running_;
    }

    bool BasicOpenXrWrapper::IsSessionFocused() const {
        return xr_session_state_ == XR_SESSION_STATE_FOCUSED;
    }

    bool BasicOpenXrWrapper::CheckBlendMode(XrEnvironmentBlendMode bldMode) {
        uint32_t count;
        CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(xr_instance_, xr_system_id_, config_view_config_type_, 0, &count,
                                                     nullptr));
        CHECK(count > 0);

        std::vector<XrEnvironmentBlendMode> blendModes(count);
        CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(xr_instance_, xr_system_id_, config_view_config_type_, count,
                                                     &count, blendModes.data()));
        acceptable_blend_modes_.insert(blendModes.begin(), blendModes.end());
        if (std::find(acceptable_blend_modes_.begin(), acceptable_blend_modes_.end(), bldMode) !=
            acceptable_blend_modes_.end()) {
            return true;
        }

        THROW("CheckBlendMode failed，No match blend mode from the xrEnumerateEnvironmentBlendModes");
        return false;
    }

    void BasicOpenXrWrapper::PollActions() {
        if (!use_input_handling_) {
            PLOGI("skip PollActions because use_input_handling_ is false");
            return;
        }

        // Base section
        {
            // sync action
            const XrActiveActionSet activeActionSet{input_.action_set, XR_NULL_PATH};
            XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
            syncInfo.countActiveActionSets = 1;
            syncInfo.activeActionSets = &activeActionSet;
            auto ret = xrSyncActions(xr_session_, &syncInfo);
            if (0 > ret) {
                PLOGE("BasicOpenXrWrapper::PollActions xrSyncActions  ret = %d", ret);
            }

            // Get pose and grab action state and start haptic vibrate when hand is 90% squeezed.
            for (auto hand : {Side::LEFT, Side::RIGHT}) {
                XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                getInfo.subactionPath = input_.controller_subaction_paths[hand];
                if (-1 == controller_type_) {
                    GetControllerType();
                }

                // update all buttons and touches of last frame
                current_frame_in_.last_frame_all_buttons = last_frame_all_buttons_;
                current_frame_in_.last_frame_all_touches = last_frame_all_touches_;
                // clear all buttons and touchs at this frame
                current_frame_in_.all_buttons_bitmask = 0u;
                current_frame_in_.all_touches_bitmask = 0u;

                // update action state for simple controller
                if (Simple_Controller == controller_type_) {
                    getInfo.action = input_.grab_action;
                    getInfo.subactionPath = input_.controller_subaction_paths[hand];
                    XrActionStateFloat grabValue{XR_TYPE_ACTION_STATE_FLOAT};
                    CHECK_XRCMD(xrGetActionStateFloat(xr_session_, &getInfo, &grabValue));
                    if (grabValue.isActive == XR_TRUE) {
                        // Scale the rendered hand by 1.0f (open) to 0.5f (fully squeezed).
                        input_.controller_scales[hand] = 1.0f - 0.5f * grabValue.currentState;
                        current_frame_in_.controller_trigger_value[hand] = grabValue.currentState;
                        controller_input_value_[hand].trigger_value = grabValue.currentState;
                        if (grabValue.currentState > 0.9f) {
                            XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
                            vibration.amplitude = 0.5;
                            vibration.duration = XR_MIN_HAPTIC_DURATION;
                            vibration.frequency = XR_FREQUENCY_UNSPECIFIED;
                            controller_input_value_[hand].trigger_click_value = true;
                            if (hand == Side::LEFT) {
                                current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonTriggerLeft;
                            } else if (hand == Side::RIGHT) {
                                current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonTriggerRight;
                            }

                            XrHapticActionInfo hapticActionInfo{XR_TYPE_HAPTIC_ACTION_INFO};
                            hapticActionInfo.action = input_.vibrate_action;
                            hapticActionInfo.subactionPath = input_.controller_subaction_paths[hand];
                            CHECK_XRCMD(xrApplyHapticFeedback(xr_session_, &hapticActionInfo,
                                                              reinterpret_cast<XrHapticBaseHeader*>(&vibration)));
                        }
                    }

                    getInfo.action = input_.grip_pose_action;
                    getInfo.subactionPath = input_.controller_subaction_paths[hand];
                    XrActionStatePose poseState{XR_TYPE_ACTION_STATE_POSE};
                    CHECK_XRCMD(xrGetActionStatePose(xr_session_, &getInfo, &poseState));
                    input_.controller_actives[hand] = poseState.isActive;
                    current_frame_in_.controller_actives[hand] = poseState.isActive;

                    // Update the hand pose and aim pose.
                    if (poseState.isActive) {
                        XrSpaceLocation handLocation{XR_TYPE_SPACE_LOCATION};
                        auto res = xrLocateSpace(input_.controller_grip_spaces[hand], app_space_,
                                                 current_frame_in_.predicted_display_time, &handLocation);
                        CHECK_XRRESULT(res, Fmt("xrLocateSpace at hand space: %d", hand).c_str());
                        if (XR_UNQUALIFIED_SUCCESS(res)) {
                            if ((handLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                                (handLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
                                current_frame_in_.controller_poses[hand] = handLocation.pose;
                            }
                        }
                        // Update the hand aim pose.
                        res = xrLocateSpace(input_.controller_aim_spaces[hand], app_space_,
                                            current_frame_in_.predicted_display_time, &handLocation);
                        CHECK_XRRESULT(res, Fmt("xrLocateSpace at aim space: %d", hand).c_str());
                        if (XR_UNQUALIFIED_SUCCESS(res)) {
                            if ((handLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                                (handLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
                                current_frame_in_.controller_aim_poses[hand] = handLocation.pose;
                            }
                        }
                    } else {
                        PLOGI("Hand %d is not active, skip locate to get pose", hand);
                    }
                }

                if ((XR_CV3_Optics_Controller_Type == controller_type_) ||
                    (XR_CV3_Phoenix_Controller_Type == controller_type_) ||
                    (XR_CV3_Hawk_Controller_Type == controller_type_) || (Simple_Controller == controller_type_) ||
                    (XR_CV3_MerlinE_Controller_Type == controller_type_)) {
                    {
                        //controller_battery_value
                        if (Simple_Controller != controller_type_) {
                            getInfo.action = input_.battery_action;
                            XrActionStateFloat batteryValue{XR_TYPE_ACTION_STATE_FLOAT};
                            ret = xrGetActionStateFloat(xr_session_, &getInfo, &batteryValue);
                            if (0 > ret) {
                                PLOGE("xrGetActionState controller_battery_value  ret = %d", ret);
                            }
                            if (batteryValue.isActive == XR_TRUE) {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].battery_value = batteryValue.currentState;
                                    current_frame_in_.controller_battery_value[Side::LEFT] = batteryValue.currentState;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].battery_value = batteryValue.currentState;
                                    current_frame_in_.controller_battery_value[Side::RIGHT] = batteryValue.currentState;
                                }
                            }
                        }

                        //home_value
                        {
                            getInfo.action = input_.home_action;
                            XrActionStateBoolean homeValue{XR_TYPE_ACTION_STATE_BOOLEAN};
                            ret = xrGetActionStateBoolean(xr_session_, &getInfo, &homeValue);
                            if (0 > ret) {
                                PLOGE("xrGetActionState home_value  ret = %d", ret);
                            }
                            if (homeValue.isActive == XR_TRUE) {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].home_value = homeValue.currentState;
                                    if (XR_TRUE == homeValue.currentState) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonMenu;
                                    }
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].home_value = homeValue.currentState;
                                    if (XR_TRUE == homeValue.currentState) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonHome;
                                    }
                                }
                                if (homeValue.changedSinceLastSync == XR_TRUE && homeValue.currentState == XR_TRUE) {
                                    PLOGD("key event  home key click %d", hand);
                                }
                            } else {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].home_value = 0;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].home_value = 0;
                                }
                            }
                        }
                        //back_value
                        {
                            getInfo.action = input_.back_action;
                            XrActionStateBoolean backValue{XR_TYPE_ACTION_STATE_BOOLEAN};
                            ret = xrGetActionStateBoolean(xr_session_, &getInfo, &backValue);
                            if (0 > ret) {
                                PLOGE("xrGetActionState back_value  ret = %d", ret);
                            }
                            if (backValue.isActive == XR_TRUE) {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].back_value = backValue.currentState;
                                    if (XR_TRUE == backValue.currentState) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonBackLeft;
                                    }
                                } else if (hand == 1) {
                                    if ((XR_CV3_Phoenix_Controller_Type == controller_type_) ||
                                        (XR_CV3_Hawk_Controller_Type == controller_type_)) {
                                        controller_input_value_[Side::RIGHT].back_value = 0;
                                    } else {
                                        controller_input_value_[Side::RIGHT].back_value = backValue.currentState;
                                        if (XR_TRUE == backValue.currentState) {
                                            current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonBackRight;
                                        }
                                    }
                                }
                                if (backValue.changedSinceLastSync == XR_TRUE && backValue.currentState == XR_TRUE) {
                                    PLOGD("key event  back key click %d", hand);
                                }
                            } else {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].back_value = 0;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].back_value = 0;
                                }
                            }
                        }
                        //trigger_click_value
                        if (Simple_Controller != controller_type_) {
                            getInfo.action = input_.trigger_click_action;
                            XrActionStateBoolean triggerclickValue{XR_TYPE_ACTION_STATE_BOOLEAN};
                            ret = xrGetActionStateBoolean(xr_session_, &getInfo, &triggerclickValue);
                            if (0 > ret) {
                                PLOGE("xrGetActionState trigger clickValue  ret = %d", ret);
                            }
                            if (triggerclickValue.isActive == XR_TRUE) {
                                PLOGD("GetActionState trigger clickValue  ret = %d  currentState = %u", ret,
                                      triggerclickValue.currentState);
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].trigger_click_value =
                                            triggerclickValue.currentState;
                                    if (XR_TRUE == triggerclickValue.currentState) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonTriggerLeft;
                                    }
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].trigger_click_value =
                                            triggerclickValue.currentState;
                                    if (XR_TRUE == triggerclickValue.currentState) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonTriggerRight;
                                    }
                                }
                                if (triggerclickValue.changedSinceLastSync == XR_TRUE &&
                                    triggerclickValue.currentState == XR_TRUE) {
                                    PLOGD("pico key event  trigger click click %d", hand);
                                }
                            } else {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].trigger_click_value = 0;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].trigger_click_value = 0;
                                }
                            }
                        }
                        //controller_trigger_value
                        {
                            getInfo.action = input_.trigger_action;
                            XrActionStateFloat triggerValue{XR_TYPE_ACTION_STATE_FLOAT};
                            ret = xrGetActionStateFloat(xr_session_, &getInfo, &triggerValue);
                            if (0 > ret) {
                                PLOGE("xrGetActionState controller_trigger_value  ret = %d", ret);
                            }
                            if (triggerValue.isActive == XR_TRUE) {
                                PLOGD("GetActionState controller_trigger_value  ret = %d  currentState = %f", ret,
                                      triggerValue.currentState);
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].trigger_value = triggerValue.currentState;
                                    current_frame_in_.controller_trigger_value[Side::LEFT] = triggerValue.currentState;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].trigger_value = triggerValue.currentState;
                                    current_frame_in_.controller_trigger_value[Side::RIGHT] = triggerValue.currentState;
                                }
                                //LOG("pico keyevent  trigger value %f",controller_trigger_value.currentState);
                            } else {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].trigger_value = 0;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].trigger_value = 0;
                                }
                            }
                        }
                        //touchpad_value
                        {
                            getInfo.action = input_.touchpad_action;
                            XrActionStateBoolean touchpadValue{XR_TYPE_ACTION_STATE_BOOLEAN};
                            ret = xrGetActionStateBoolean(xr_session_, &getInfo, &touchpadValue);
                            if (0 > ret) {
                                PLOGE("xrGetActionState touchpad_value  ret = %d", ret);
                            }
                            if (touchpadValue.isActive == XR_TRUE) {
                                PLOGD("rGetActionState touchpad_value  ret = %d  currentState = %u", ret,
                                      touchpadValue.currentState);
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].touchpad_value = touchpadValue.currentState;
                                    if (XR_TRUE == touchpadValue.currentState) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonTouchpadLeft;
                                    }
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].touchpad_value = touchpadValue.currentState;
                                    if (XR_TRUE == touchpadValue.currentState) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonTouchpadRight;
                                    }
                                }
                                if (touchpadValue.changedSinceLastSync == XR_TRUE &&
                                    touchpadValue.currentState == XR_TRUE) {
                                    PLOGD("pico key event touchpad_value click %d", hand);
                                }
                            } else {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].touchpad_value = 0;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].touchpad_value = 0;
                                }
                            }
                        }

                        //joystickValue thumbstick
                        {
                            getInfo.action = input_.joystick_action;
                            XrActionStateVector2f joystickValue{XR_TYPE_ACTION_STATE_VECTOR2F};
                            ret = xrGetActionStateVector2f(xr_session_, &getInfo, &joystickValue);
                            if (0 > ret) {
                                PLOGE("xrGetActionState joystickValue  ret = %d", ret);
                            }
                            if (joystickValue.isActive == XR_TRUE) {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].joystick.x = joystickValue.currentState.x;
                                    controller_input_value_[Side::LEFT].joystick.y = joystickValue.currentState.y;
                                    current_frame_in_.left_joystick_position.x = joystickValue.currentState.x;
                                    current_frame_in_.left_joystick_position.y = joystickValue.currentState.y;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].joystick.x = joystickValue.currentState.x;
                                    controller_input_value_[Side::RIGHT].joystick.y = joystickValue.currentState.y;
                                    current_frame_in_.right_joystick_position.x = joystickValue.currentState.x;
                                    current_frame_in_.right_joystick_position.y = joystickValue.currentState.y;
                                }
                            } else {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].joystick.x = 0;
                                    controller_input_value_[Side::LEFT].joystick.y = 0;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].joystick.x = 0;
                                    controller_input_value_[Side::RIGHT].joystick.y = 0;
                                }
                            }
                        }
                    }
                }

                if ((XR_CV3_Optics_Controller_Type == controller_type_) ||
                    (XR_CV3_Phoenix_Controller_Type == controller_type_) || (Simple_Controller == controller_type_) ||
                    (XR_CV3_Hawk_Controller_Type == controller_type_)) {
                    {
                        //TriggerTouch
                        {
                            getInfo.action = input_.trigger_touch_action;
                            XrActionStateBoolean TriggerTouch{XR_TYPE_ACTION_STATE_BOOLEAN};
                            ret = xrGetActionStateBoolean(xr_session_, &getInfo, &TriggerTouch);
                            if (0 > ret) {
                                PLOGE("xrGetActionState TriggerTouch  ret = %d", ret);
                            }
                            if (TriggerTouch.isActive == XR_TRUE) {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].trigger_touch_value = TriggerTouch.currentState;
                                    if (XR_TRUE == TriggerTouch.currentState) {
                                        current_frame_in_.all_touches_bitmask |= XrFrameIn::kTouchTriggerLeft;
                                    }
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].trigger_touch_value =
                                            TriggerTouch.currentState;
                                    if (XR_TRUE == TriggerTouch.currentState) {
                                        current_frame_in_.all_touches_bitmask |= XrFrameIn::kTouchTriggerRight;
                                    }
                                }
                                if (TriggerTouch.changedSinceLastSync == XR_TRUE &&
                                    TriggerTouch.currentState == XR_TRUE) {
                                    PLOGD("key event  TriggerTouch click %d", hand);
                                }
                            } else {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].trigger_touch_value = 0;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].trigger_touch_value = 0;
                                }
                            }
                        }

                        //sideclickValue
                        if (Simple_Controller != controller_type_) {
                            getInfo.action = input_.side_action;
                            XrActionStateBoolean sideValue{XR_TYPE_ACTION_STATE_BOOLEAN};
                            ret = xrGetActionStateBoolean(xr_session_, &getInfo, &sideValue);
                            if (0 > ret) {
                                PLOGE("xrGetActionState side_value  ret = %d", ret);
                            }
                            if (sideValue.isActive == XR_TRUE) {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].side_value = sideValue.currentState;
                                    if (XR_TRUE == sideValue.currentState) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonSideLeft;
                                    }
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].side_value = sideValue.currentState;
                                    if (XR_TRUE == sideValue.currentState) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonSideRight;
                                    }
                                }
                                if (sideValue.changedSinceLastSync == XR_TRUE && sideValue.currentState == XR_TRUE) {
                                    PLOGD("key event  side key click %d", hand);
                                }
                            } else {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].side_value = 0;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].side_value = 0;
                                }
                            }
                        }

                        //controller_grip_value
                        {
                            getInfo.action = input_.grip_action;
                            XrActionStateFloat gripValue{XR_TYPE_ACTION_STATE_FLOAT};
                            ret = xrGetActionStateFloat(xr_session_, &getInfo, &gripValue);
                            if (0 > ret) {
                                PLOGE("xrGetActionState controller_grip_value  ret = %d", ret);
                            }
                            if (gripValue.isActive == XR_TRUE) {
                                PLOGD("GetActionState controller_grip_value  ret = %d  currentState = %f", ret,
                                      gripValue.currentState);
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].grip_value = gripValue.currentState;
                                    current_frame_in_.controller_grip_value[Side::LEFT] = gripValue.currentState;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].grip_value = gripValue.currentState;
                                    current_frame_in_.controller_grip_value[Side::RIGHT] = gripValue.currentState;
                                }
                            } else {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].grip_value = 0;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].grip_value = 0;
                                }
                            }
                        }
                        //RockerTouch
                        if (Simple_Controller != controller_type_) {
                            getInfo.action = input_.rocker_touch_action;
                            XrActionStateBoolean RockerTouch{XR_TYPE_ACTION_STATE_BOOLEAN};
                            ret = xrGetActionStateBoolean(xr_session_, &getInfo, &RockerTouch);
                            if (0 > ret) {
                                PLOGE("xrGetActionState RockerTouch  ret = %d", ret);
                            }
                            if (RockerTouch.isActive == XR_TRUE) {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].rocker_touch_value = RockerTouch.currentState;
                                    if (XR_TRUE == RockerTouch.currentState) {
                                        current_frame_in_.all_touches_bitmask |= XrFrameIn::kTouchRockerLeft;
                                    }
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].rocker_touch_value = RockerTouch.currentState;
                                    if (XR_TRUE == RockerTouch.currentState) {
                                        current_frame_in_.all_touches_bitmask |= XrFrameIn::kTouchRockerRight;
                                    }
                                }
                                if (RockerTouch.changedSinceLastSync == XR_TRUE &&
                                    RockerTouch.currentState == XR_TRUE) {
                                    PLOGD("key event  RockerTouch click %d", hand);
                                }
                            } else {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].rocker_touch_value = 0;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].rocker_touch_value = 0;
                                }
                            }
                        }

                        //ThumbrestTouch
                        {
                            getInfo.action = input_.thumb_rest_touch_action;
                            XrActionStateBoolean ThumbrestTouch{XR_TYPE_ACTION_STATE_BOOLEAN};
                            ret = xrGetActionStateBoolean(xr_session_, &getInfo, &ThumbrestTouch);
                            if (0 > ret) {
                                PLOGE("xrGetActionState Thumbrest Touch  ret = %d", ret);
                            }
                            if (ThumbrestTouch.isActive == XR_TRUE) {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].thumb_rest_touch_value =
                                            ThumbrestTouch.currentState;
                                    if (XR_TRUE == ThumbrestTouch.currentState) {
                                        current_frame_in_.all_touches_bitmask |= XrFrameIn::kTouchThumbRestLeft;
                                    }
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].thumb_rest_touch_value =
                                            ThumbrestTouch.currentState;
                                    if (XR_TRUE == ThumbrestTouch.currentState) {
                                        current_frame_in_.all_touches_bitmask |= XrFrameIn::kTouchThumbRestRight;
                                    }
                                }
                                if (ThumbrestTouch.changedSinceLastSync == XR_TRUE &&
                                    ThumbrestTouch.currentState == XR_TRUE) {
                                    PLOGD("key event  Thumbrest Touch click %d", hand);
                                }
                            } else {
                                if (hand == 0) {
                                    controller_input_value_[Side::LEFT].thumb_rest_touch_value = 0;
                                } else if (hand == 1) {
                                    controller_input_value_[Side::RIGHT].thumb_rest_touch_value = 0;
                                }
                            }
                        }

                        //XValue YValue X touch Y touch A Value B Value A touch B touch
                        {
                            if (hand == 0) {
                                getInfo.action = input_.X_action;
                                XrActionStateBoolean XValue{XR_TYPE_ACTION_STATE_BOOLEAN};
                                ret = xrGetActionStateBoolean(xr_session_, &getInfo, &XValue);
                                if (0 > ret) {
                                    PLOGE("xrGetActionState X Value  ret = %d", ret);
                                }
                                if (XValue.isActive == XR_TRUE) {
                                    controller_input_value_[Side::LEFT].A_X_value = XValue.currentState;
                                    if (XValue.currentState == XR_TRUE) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonX;
                                    }

                                    if (XValue.changedSinceLastSync == XR_TRUE && XValue.currentState == XR_TRUE) {
                                        PLOGD("pico key event  X key click");
                                    }
                                } else {
                                    controller_input_value_[Side::LEFT].A_X_value = 0;
                                }

                                getInfo.action = input_.Y_action;
                                XrActionStateBoolean YValue{XR_TYPE_ACTION_STATE_BOOLEAN};
                                ret = xrGetActionStateBoolean(xr_session_, &getInfo, &YValue);
                                if (0 > ret) {
                                    PLOGE("xrGetActionState Y Value  ret = %d", ret);
                                }
                                if (YValue.isActive == XR_TRUE) {
                                    controller_input_value_[Side::LEFT].B_Y_value = YValue.currentState;
                                    if (YValue.currentState == XR_TRUE) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonY;
                                    }

                                    if (YValue.changedSinceLastSync == XR_TRUE && YValue.currentState == XR_TRUE) {
                                        PLOGD("pico key event  BY key click");
                                    }
                                } else {
                                    controller_input_value_[Side::LEFT].B_Y_value = 0;
                                }

                                getInfo.action = input_.X_touch_action;
                                XrActionStateBoolean Xtouch{XR_TYPE_ACTION_STATE_BOOLEAN};
                                ret = xrGetActionStateBoolean(xr_session_, &getInfo, &Xtouch);
                                if (0 > ret) {
                                    PLOGE("xrGetActionState X touch  ret = %d", ret);
                                }
                                if (Xtouch.isActive == XR_TRUE) {
                                    controller_input_value_[Side::LEFT].A_X_touch_value = Xtouch.currentState;
                                    if (Xtouch.currentState == XR_TRUE) {
                                        current_frame_in_.all_touches_bitmask |= XrFrameIn::kTouchX;
                                    }

                                    if (Xtouch.changedSinceLastSync == XR_TRUE && Xtouch.currentState == XR_TRUE) {
                                        PLOGD("pico key event X touch click");
                                    }
                                } else {
                                    controller_input_value_[Side::LEFT].A_X_touch_value = 0;
                                }

                                getInfo.action = input_.Y_touch_action;
                                XrActionStateBoolean Ytouch{XR_TYPE_ACTION_STATE_BOOLEAN};
                                ret = xrGetActionStateBoolean(xr_session_, &getInfo, &Ytouch);
                                if (0 > ret) {
                                    PLOGE("xrGetActionState Y touch  ret = %d", ret);
                                }
                                if (Ytouch.isActive == XR_TRUE) {
                                    controller_input_value_[Side::LEFT].B_Y_touch_value = Ytouch.currentState;
                                    if (Ytouch.currentState == XR_TRUE) {
                                        current_frame_in_.all_touches_bitmask |= XrFrameIn::kTouchY;
                                    }
                                    if (Ytouch.changedSinceLastSync == XR_TRUE && Ytouch.currentState == XR_TRUE) {
                                        PLOGD("pico key event  Y touch click");
                                    }
                                } else {
                                    controller_input_value_[Side::LEFT].B_Y_touch_value = 0;
                                }
                            } else if (hand == 1) {
                                getInfo.action = input_.B_action;
                                XrActionStateBoolean BValue{XR_TYPE_ACTION_STATE_BOOLEAN};
                                ret = xrGetActionStateBoolean(xr_session_, &getInfo, &BValue);
                                if (0 > ret) {
                                    PLOGE("xrGetActionState BValue  ret = %d", ret);
                                }
                                if (BValue.isActive == XR_TRUE) {
                                    controller_input_value_[Side::RIGHT].B_Y_value = BValue.currentState;
                                    if (BValue.currentState == XR_TRUE) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonB;
                                    }

                                    if (BValue.changedSinceLastSync == XR_TRUE && BValue.currentState == XR_TRUE) {
                                        PLOGD("pico key event  B key click");
                                    }
                                } else {
                                    controller_input_value_[Side::RIGHT].B_Y_value = 0;
                                }

                                getInfo.action = input_.A_action;
                                XrActionStateBoolean AValue{XR_TYPE_ACTION_STATE_BOOLEAN};
                                ret = xrGetActionStateBoolean(xr_session_, &getInfo, &AValue);
                                if (0 > ret) {
                                    PLOGE("xrGetActionState AValue  ret = %d", ret);
                                }
                                if (AValue.isActive == XR_TRUE) {
                                    controller_input_value_[Side::RIGHT].A_X_value = AValue.currentState;
                                    if (AValue.currentState == XR_TRUE) {
                                        current_frame_in_.all_buttons_bitmask |= XrFrameIn::kButtonA;
                                    }

                                    if (AValue.changedSinceLastSync == XR_TRUE && AValue.currentState == XR_TRUE) {
                                        PLOGD("pico key event  A key click");
                                    }
                                } else {
                                    controller_input_value_[Side::RIGHT].A_X_value = 0;
                                }

                                getInfo.action = input_.A_touch_action;
                                XrActionStateBoolean Atouch{XR_TYPE_ACTION_STATE_BOOLEAN};
                                ret = xrGetActionStateBoolean(xr_session_, &getInfo, &Atouch);
                                if (0 > ret) {
                                    PLOGE("xrGetActionState A touch  ret = %d", ret);
                                }
                                if (Atouch.isActive == XR_TRUE) {
                                    controller_input_value_[Side::RIGHT].A_X_touch_value = Atouch.currentState;
                                    if (Atouch.currentState == XR_TRUE) {
                                        current_frame_in_.all_touches_bitmask |= XrFrameIn::kTouchA;
                                    }
                                    if (Atouch.changedSinceLastSync == XR_TRUE && Atouch.currentState == XR_TRUE) {
                                        PLOGD("pico key event  A touch click");
                                    }
                                } else {
                                    controller_input_value_[Side::RIGHT].A_X_touch_value = 0;
                                }

                                getInfo.action = input_.B_touch_action;
                                XrActionStateBoolean Btouch{XR_TYPE_ACTION_STATE_BOOLEAN};
                                ret = xrGetActionStateBoolean(xr_session_, &getInfo, &Btouch);
                                if (0 > ret) {
                                    PLOGE("xrGetActionState B touch  ret = %d", ret);
                                }
                                if (Btouch.isActive == XR_TRUE) {
                                    controller_input_value_[Side::RIGHT].B_Y_touch_value = Btouch.currentState;
                                    if (Btouch.currentState == XR_TRUE) {
                                        current_frame_in_.all_touches_bitmask |= XrFrameIn::kTouchB;
                                    }
                                    if (Btouch.changedSinceLastSync == XR_TRUE && Btouch.currentState == XR_TRUE) {
                                        PLOGD("key event  B touch click");
                                    }
                                } else {
                                    controller_input_value_[Side::RIGHT].B_Y_touch_value = 0;
                                }
                            }
                        }
                    }
                }

                if ((XR_CV3_Optics_Controller_Type == controller_type_) ||
                    (XR_CV3_Phoenix_Controller_Type == controller_type_) || (Simple_Controller == controller_type_) ||
                    (XR_CV3_Hawk_Controller_Type == controller_type_)) {
                    // Update the hand pose.
                    getInfo.action = input_.grip_pose_action;
                    getInfo.subactionPath = input_.controller_subaction_paths[hand];
                    XrActionStatePose poseState{XR_TYPE_ACTION_STATE_POSE};
                    CHECK_XRCMD(xrGetActionStatePose(xr_session_, &getInfo, &poseState));
                    input_.controller_actives[hand] = poseState.isActive;
                    current_frame_in_.controller_actives[hand] = poseState.isActive;

                    if (poseState.isActive) {
                        XrSpaceLocation handLocation{XR_TYPE_SPACE_LOCATION};
                        auto res = xrLocateSpace(input_.controller_grip_spaces[hand], app_space_,
                                                 current_frame_in_.predicted_display_time, &handLocation);
                        CHECK_XRRESULT(res, Fmt("xrLocateSpace at hand space: %d", hand).c_str());
                        if (XR_UNQUALIFIED_SUCCESS(res)) {
                            if ((handLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                                (handLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
                                current_frame_in_.controller_poses[hand] = handLocation.pose;
                            }
                        }
                        // Update the hand aim pose.
                        res = xrLocateSpace(input_.controller_aim_spaces[hand], app_space_,
                                            current_frame_in_.predicted_display_time, &handLocation);
                        CHECK_XRRESULT(res, Fmt("xrLocateSpace at aim space: %d", hand).c_str());
                        if (XR_UNQUALIFIED_SUCCESS(res)) {
                            if ((handLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                                (handLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
                                current_frame_in_.controller_aim_poses[hand] = handLocation.pose;
                            }
                        }
                    } else {
                        //                        PLOGV("Hand %d is not active, skip locate to get pose", hand);
                    }
                }
            }
            //eyetracking
            if (eye_tracking_supported_) {
                XrActionStatePose actionStatePose{XR_TYPE_ACTION_STATE_POSE};
                XrActionStateGetInfo getActionStateInfo{XR_TYPE_ACTION_STATE_GET_INFO};
                getActionStateInfo.action = input_.gaze_action;
                CHECK_XRCMD(xrGetActionStatePose(xr_session_, &getActionStateInfo, &actionStatePose));
                input_.gaze_active = actionStatePose.isActive;
            }

            // update last frame input
            last_frame_all_buttons_ = current_frame_in_.all_buttons_bitmask;
            last_frame_all_touches_ = current_frame_in_.all_touches_bitmask;
        }

        /// extension features cb
        auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
        for (auto& feature : extensions) {
            feature->OnActionsPoll();
        }

        // customized poll actions
        if (!CustomizedActionsPoll()) {
            PLOGE("CustomizedPollActions failed");
            THROW_XR(XR_ERROR_RUNTIME_FAILURE, "CustomizedPollActions failed");
        }

        // process application-specific input logic.
        if (!CustomizedHandleInput()) {
            PLOGE("CustomizedHandleInput failed");
            THROW_XR(XR_ERROR_RUNTIME_FAILURE, "CustomizedHandleInput failed");
        }

        /// TODO: process registered input chock.
        if (!input_callbacks_.empty()) {
            for (auto& callback : input_callbacks_) {
                callback(this, current_frame_in_);
            }
        }
    }

    void BasicOpenXrWrapper::DoFrame() {
        CHECK(xr_session_ != XR_NULL_HANDLE);

        /// count frame number
        current_frame_in_.frame_number++;

        /// Just pin it
        // Updates the stage bound status. Temporarily useless.
        if (is_stage_bound_dirty_) {
            XrExtent2Df stageBounds = {};
            XrResult result;
            CHECK_XRCMD(xrGetReferenceSpaceBoundsRect(xr_session_, XR_REFERENCE_SPACE_TYPE_STAGE, &stageBounds));
            is_stage_bound_dirty_ = false;
        }

        /// Wait frame
        XrFrameWaitInfo waitFrameInfo = {XR_TYPE_FRAME_WAIT_INFO};
        // Wait for the next frame.
        XrFrameState frameState = {XR_TYPE_FRAME_STATE};

        /// extension features cb
        auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
        for (auto& feature : extensions) {
            feature->OnPreWaitFrame();
        }

        if (!CustomizedPreWaitFrame(waitFrameInfo, frameState)) {
            PLOGE("CustomizedPreWaitFrame failed");
            THROW_XR(XR_ERROR_RUNTIME_FAILURE, "CustomizedPreWaitFrame failed");
        }

        CHECK_XRCMD(xrWaitFrame(xr_session_, &waitFrameInfo, &frameState));

        /// extension features cb
        for (auto& feature : extensions) {
            feature->OnPostWaitFrame();
        }

        if (!CustomizedPostWaitFrame(waitFrameInfo, frameState)) {
            PLOGE("CustomizedPostWaitFrame failed");
            THROW_XR(XR_ERROR_RUNTIME_FAILURE, "CustomizedPostWaitFrame failed");
        }

        /// extension features cb
        for (auto& feature : extensions) {
            feature->OnPreBeginFrame();
        }

        /// Begin frame
        XrFrameBeginInfo frameBeginInfo = {XR_TYPE_FRAME_BEGIN_INFO};
        CHECK_XRCMD(xrBeginFrame(xr_session_, &frameBeginInfo));
        bool shouldRender = frameState.shouldRender;

        /// extension features cb
        for (auto& feature : extensions) {
            feature->OnPostBeginFrame();
        }

        /// Locate head space and Locate views
        // Get the HMD pose at the middle of the frame. The number of frames predicted ahead depends on the
        // the pipeline depth of the engine_ and the synthesis rate.
        // The better the prediction, the less black will be pulled in at the edges.
        XrSpaceLocation hmdLocation = {XR_TYPE_SPACE_LOCATION};
        CHECK_XRCMD(xrLocateSpace(head_view_space_, app_space_, frameState.predictedDisplayTime, &hmdLocation));
        XrPosef headPoseAtAppSpc = hmdLocation.pose;

        // Locate views to get the projection matrices to head space.
        // Populate the view state.
        XrViewState viewState = {XR_TYPE_VIEW_STATE};
        XrViewLocateInfo projectionViewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
        projectionViewLocateInfo.viewConfigurationType = config_view_config_type_;
        projectionViewLocateInfo.displayTime = frameState.predictedDisplayTime;
        // Use head space to locate the views. Then render layers use head space too;
        projectionViewLocateInfo.space = head_view_space_;
        uint32_t projectionCapacityInput = static_cast<uint32_t>(projection_views_cache_.size());
        uint32_t projectionCountOutput;
        auto res = xrLocateViews(xr_session_, &projectionViewLocateInfo, &viewState, projectionCapacityInput,
                                 &projectionCountOutput, projection_views_cache_.data());
        CHECK_XRRESULT(res, "xrLocateViews");
        CHECK(projectionCountOutput == projectionCapacityInput);
        CHECK(projectionCountOutput == config_views_.size());
        CHECK(projectionCountOutput == swapchains_.size());

        /// time accounting
        current_frame_in_.predicted_display_time = frameState.predictedDisplayTime;
        if (current_frame_in_.previous_display_time > 0) {
            current_frame_in_.delta_time_in_seconds =
                    current_frame_in_.predicted_display_time - current_frame_in_.previous_display_time;
        }
        current_frame_in_.previous_display_time = current_frame_in_.predicted_display_time;

        /// TODO: calculate the view pose and view projection matrix, and populate FrameIn.
        for (size_t eye = 0; eye < config_views_.size(); eye++) {
            XrPosef eyeAtHead = projection_views_cache_[eye].pose;
            XrPosef eyeAtAppSpc{};
            XrPosef_Multiply(&eyeAtAppSpc, &headPoseAtAppSpc, &eyeAtHead);
            current_frame_in_.views[eye].pose = eyeAtAppSpc;
            current_frame_in_.views[eye].fov = projection_views_cache_[eye].fov;
            current_frame_in_.head_pose = headPoseAtAppSpc;
        }

        /// Poll actions.
        PollActions();

        /// Render frame
        if (shouldRender) {
            RenderFrame();
        }

        /// Commit frame
        std::vector<XrCompositionLayerBaseHeader*> layers;
        for (uint32_t i = 0; i < comp_layer_count_; i++) {
            // layers[i] = (XrCompositionLayerBaseHeader*)&comp_layers_map_[i];
            layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&comp_layers_[i]));
        }

        /// extension features cb
        for (auto& feature : extensions) {
            feature->OnPreEndFrame(&layers);
        }

        /// End frame
        XrFrameEndInfo frameEndInfo = {XR_TYPE_FRAME_END_INFO};
        frameEndInfo.displayTime = frameState.predictedDisplayTime;
        frameEndInfo.environmentBlendMode = xr_environment_blend_mode_;
        comp_layer_count_ = static_cast<uint32_t>(layers.size());
        frameEndInfo.layerCount = comp_layer_count_;
        frameEndInfo.layers = layers.data();

        // CHECK_XRCMD(xrEndFrame(xr_session_, &frameEndInfo));

        auto ret = xrEndFrame(xr_session_, &frameEndInfo);
        if (XR_SUCCESS != ret) {
            PLOGE("this frame xrEndFrame failed, ret: %s", to_string(ret));
        }

        /// extension features cb
        for (auto& feature : extensions) {
            feature->OnPostEndFrame(&layers);
        }
    }

    bool BasicOpenXrWrapper::RegisterHandleInputFunc(InputHandlerFunc handleInputFunc) {
        if (handleInputFunc == nullptr) {
            PLOGE("handleInputFunc is nullptr");
            return false;
        }
        this->input_callbacks_.push_back(handleInputFunc);
        return true;
    }

    void BasicOpenXrWrapper::RenderFrame() {
        /// reset
        comp_layer_count_ = 0;
        memset(comp_layers_, 0, sizeof(PVRSampleFW::XrCompositionLayerUnion) * MAX_NUM_COMPOSITION_LAYERS);

        /// pre render frame
        if (!CustomizedPreRenderFrame()) {
            PLOGE("CustomizedPreRenderFrame failed");
            THROW_XR(XR_ERROR_RUNTIME_FAILURE, "CustomizedPreRenderFrame failed");
        }

        if (!CustomizedRender()) {
            PLOGE("CustomizedRenderScene failed!");
        }

        /// post render frame
        if (!CustomizedPostRenderFrame()) {
            PLOGE("CustomizedPostRenderFrame failed");
            THROW_XR(XR_ERROR_RUNTIME_FAILURE, "CustomizedPostRenderFrame failed");
        }
    }

    bool BasicOpenXrWrapper::DestroySession() {
        // destroy swapchains_
        for (auto& swapchain : swapchains_) {
            // free swapchain images vector memory
            std::vector<XrSwapchainImageBaseHeader*>().swap(swapchain_images_[swapchain.handle]);
            CHECK_XRCMD(xrDestroySwapchain(swapchain.handle));
        }
        // free swapchains_ vector memory
        std::vector<Swapchain>().swap(swapchains_);

        // free the views vector memory
        std::vector<XrView>().swap(projection_views_cache_);
        std::vector<XrViewConfigurationView>().swap(config_views_);

        /// TODO: consider to destroy the action set when the instance is destroyed
        ///  when multiple sessions are created.
        // destroy action set
        CHECK_XRCMD(xrDestroyActionSet(input_.action_set));

        // destroy spaces
        CHECK_XRCMD(xrDestroySpace(head_view_space_));
        CHECK_XRCMD(xrDestroySpace(app_space_));
        // destroy session
        CHECK_XRCMD(xrDestroySession(xr_session_));

        /// extension features cb
        auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
        for (auto& feature : extensions) {
            feature->OnSessionDestroy();
        }
        return true;
    }

    bool BasicOpenXrWrapper::DestroyInstance() {
        // free extensions vector memory
        std::vector<XrExtensionProperties>().swap(runtime_supported_extensions_);
        std::vector<std::string>().swap(enabled_extensions_);

        // destroy instance
        CHECK_XRCMD(xrDestroyInstance(xr_instance_));

        /// extension features cb
        auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
        for (auto& feature : extensions) {
            feature->OnInstanceDestroy();
        }
        return true;
    }

    bool BasicOpenXrWrapper::InitializeActions() {
        PLOGI("InitializeActions Start.\n");

        if (!use_input_handling_) {
            PLOGI("InitializeActions skip, use_input_handling_ is false.\n");
            return true;
        }

        // Base section
        {
            // create action set
            input_.action_set = CreateActionSet(0, "piconativesdk", "PicoNativeSDK");

            // create subaction paths for left and right hands.
            CHECK_XRCMD(
                    xrStringToPath(xr_instance_, "/user/hand/left", &input_.controller_subaction_paths[Side::LEFT]));
            CHECK_XRCMD(
                    xrStringToPath(xr_instance_, "/user/hand/right", &input_.controller_subaction_paths[Side::RIGHT]));

            // create actions
            {
                // Create an input action for grabbing objects with the left and right hands.
                input_.grab_action = CreateAction(input_.action_set, XR_ACTION_TYPE_FLOAT_INPUT, "grab_object",
                                                  "Grab Object", Side::COUNT, input_.controller_subaction_paths.data());
                // Create an input action getting the left and right aim poses.
                input_.aim_pose_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_POSE_INPUT, "aim_pose", "Aim Pose", Side::COUNT,
                                     input_.controller_subaction_paths.data());
                // Create an input action getting the left and right hand poses.
                input_.grip_pose_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_POSE_INPUT, "hand_pose", "Hand Pose",
                                     Side::COUNT, input_.controller_subaction_paths.data());
                // Create output actions for vibrating the left and right controller.
                input_.vibrate_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_VIBRATION_OUTPUT, "vibrate_hand", "Vibrate Hand",
                                     Side::COUNT, input_.controller_subaction_paths.data());
                // Create input actions for quitting the session using the left and right controller.
                input_.quit_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "quit_session", "Quit Session",
                                     Side::COUNT, input_.controller_subaction_paths.data());

                /**********************************pico***************************************/
                // Create input actions for toucpad key using the left and right controller.
                input_.touchpad_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "touchpad", "Touchpad",
                                     Side::COUNT, input_.controller_subaction_paths.data());

                input_.A_X_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "axkey", "AXKey",
                                                 Side::COUNT, input_.controller_subaction_paths.data());

                input_.home_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "homekey",
                                                  "Home Key", Side::COUNT, input_.controller_subaction_paths.data());

                input_.B_Y_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "bykey", "BYKey",
                                                 Side::COUNT, input_.controller_subaction_paths.data());

                input_.back_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "backkey",
                                                  "Back Key", Side::COUNT, input_.controller_subaction_paths.data());

                input_.side_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "sidekey",
                                                  "Side Key", Side::COUNT, input_.controller_subaction_paths.data());

                input_.trigger_action = CreateAction(input_.action_set, XR_ACTION_TYPE_FLOAT_INPUT, "trigger",
                                                     "Trigger", Side::COUNT, input_.controller_subaction_paths.data());

                input_.joystick_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_VECTOR2F_INPUT, "joystick", "Joystick",
                                     Side::COUNT, input_.controller_subaction_paths.data());

                input_.battery_action = CreateAction(input_.action_set, XR_ACTION_TYPE_FLOAT_INPUT, "battery",
                                                     "Battery", Side::COUNT, input_.controller_subaction_paths.data());

                input_.A_X_touch_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "axtouch", "AXtouch", Side::COUNT,
                                     input_.controller_subaction_paths.data());

                input_.B_Y_touch_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "bytouch", "BYtouch", Side::COUNT,
                                     input_.controller_subaction_paths.data());

                input_.rocker_touch_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "rockertouch", "Rockertouch",
                                     Side::COUNT, input_.controller_subaction_paths.data());

                input_.trigger_touch_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "triggertouch", "Triggertouch",
                                     Side::COUNT, input_.controller_subaction_paths.data());

                input_.thumb_rest_touch_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "thumbresttouch",
                                     "Thumbresttouch", Side::COUNT, input_.controller_subaction_paths.data());

                input_.grip_action = CreateAction(input_.action_set, XR_ACTION_TYPE_FLOAT_INPUT, "gripvalue",
                                                  "GripValue", Side::COUNT, input_.controller_subaction_paths.data());

                input_.trigger_click_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "triggerclick", "Triggerclick",
                                     Side::COUNT, input_.controller_subaction_paths.data());

                input_.A_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "akey", "Akey",
                                               Side::COUNT, input_.controller_subaction_paths.data());

                input_.B_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "bkey", "Bkey",
                                               Side::COUNT, input_.controller_subaction_paths.data());

                input_.X_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "xkey", "Xkey",
                                               Side::COUNT, input_.controller_subaction_paths.data());

                input_.Y_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "ykey", "Ykey",
                                               Side::COUNT, input_.controller_subaction_paths.data());

                input_.A_touch_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "atouch",
                                                     "Atouch", Side::COUNT, input_.controller_subaction_paths.data());

                input_.B_touch_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "btouch",
                                                     "Btouch", Side::COUNT, input_.controller_subaction_paths.data());

                input_.X_touch_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "xtouch",
                                                     "Xtouch", Side::COUNT, input_.controller_subaction_paths.data());

                input_.Y_touch_action = CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "ytouch",
                                                     "Ytouch", Side::COUNT, input_.controller_subaction_paths.data());

                /*******************************merlineE********************************/
                input_.trackpad_click_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "trackpadclick", "Trackpadclick",
                                     Side::COUNT, input_.controller_subaction_paths.data());

                input_.trackpad_touch_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_BOOLEAN_INPUT, "trackpadtouch", "Trackpadtouch",
                                     Side::COUNT, input_.controller_subaction_paths.data());

                input_.trackpad_value_action =
                        CreateAction(input_.action_set, XR_ACTION_TYPE_VECTOR2F_INPUT, "trackpadvalue", "Trackpadvalue",
                                     Side::COUNT, input_.controller_subaction_paths.data());
                /**********************************pico***************************************/
            }

            // paths for the actions
            std::array<XrPath, Side::COUNT> selectPath;
            std::array<XrPath, Side::COUNT> squeezeValuePath;
            std::array<XrPath, Side::COUNT> squeezeClickPath;
            std::array<XrPath, Side::COUNT> aimPosePath;
            std::array<XrPath, Side::COUNT> gripPosePath;
            std::array<XrPath, Side::COUNT> hapticPath;
            std::array<XrPath, Side::COUNT> menuClickPath;
            std::array<XrPath, Side::COUNT> systemPath;
            std::array<XrPath, Side::COUNT> thumbrestPath;
            std::array<XrPath, Side::COUNT> triggerClickPath;
            std::array<XrPath, Side::COUNT> triggerTouchPath;
            std::array<XrPath, Side::COUNT> triggerValuePath;
            std::array<XrPath, Side::COUNT> thumbstickClickPath;
            std::array<XrPath, Side::COUNT> thumbstickTouchPath;
            std::array<XrPath, Side::COUNT> thumbstickPosPath;
            std::array<XrPath, Side::COUNT> batteryPath;
            std::array<XrPath, Side::COUNT> backPath;
            std::array<XrPath, Side::COUNT> AValuePath;
            std::array<XrPath, Side::COUNT> BValuePath;
            std::array<XrPath, Side::COUNT> XValuePath;
            std::array<XrPath, Side::COUNT> YValuePath;
            std::array<XrPath, Side::COUNT> ATouchPath;
            std::array<XrPath, Side::COUNT> BTouchPath;
            std::array<XrPath, Side::COUNT> XTouchPath;
            std::array<XrPath, Side::COUNT> YTouchPath;
            std::array<XrPath, Side::COUNT> TrackPadClickPath;
            std::array<XrPath, Side::COUNT> TrackPadValuePath;

            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/x/click", &XValuePath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/y/click", &YValuePath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/a/click", &AValuePath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/b/click", &BValuePath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/x/touch", &XTouchPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/y/touch", &YTouchPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/a/touch", &ATouchPath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/b/touch", &BTouchPath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/select/click", &selectPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/select/click", &selectPath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/menu/click", &menuClickPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/menu/click", &menuClickPath[Side::RIGHT]));

            CHECK_XRCMD(
                    xrStringToPath(xr_instance_, "/user/hand/left/input/squeeze/value", &squeezeValuePath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/squeeze/value",
                                       &squeezeValuePath[Side::RIGHT]));
            CHECK_XRCMD(
                    xrStringToPath(xr_instance_, "/user/hand/left/input/squeeze/click", &squeezeClickPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/squeeze/click",
                                       &squeezeClickPath[Side::RIGHT]));

            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/aim/pose", &aimPosePath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/aim/pose", &aimPosePath[Side::RIGHT]));

            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/grip/pose", &gripPosePath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/grip/pose", &gripPosePath[Side::RIGHT]));

            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/output/haptic", &hapticPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/output/haptic", &hapticPath[Side::RIGHT]));

            CHECK_XRCMD(
                    xrStringToPath(xr_instance_, "/user/hand/left/input/trigger/click", &triggerClickPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/trigger/click",
                                       &triggerClickPath[Side::RIGHT]));
            CHECK_XRCMD(
                    xrStringToPath(xr_instance_, "/user/hand/left/input/trigger/touch", &triggerTouchPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/trigger/touch",
                                       &triggerTouchPath[Side::RIGHT]));
            CHECK_XRCMD(
                    xrStringToPath(xr_instance_, "/user/hand/left/input/trigger/value", &triggerValuePath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/trigger/value",
                                       &triggerValuePath[Side::RIGHT]));

            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/thumbstick/click",
                                       &thumbstickClickPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/thumbstick/click",
                                       &thumbstickClickPath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/thumbstick/touch",
                                       &thumbstickTouchPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/thumbstick/touch",
                                       &thumbstickTouchPath[Side::RIGHT]));
            CHECK_XRCMD(
                    xrStringToPath(xr_instance_, "/user/hand/left/input/thumbstick", &thumbstickPosPath[Side::LEFT]));
            CHECK_XRCMD(
                    xrStringToPath(xr_instance_, "/user/hand/right/input/thumbstick", &thumbstickPosPath[Side::RIGHT]));

            CHECK_XRCMD(
                    xrStringToPath(xr_instance_, "/user/hand/left/input/thumbrest/touch", &thumbrestPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/thumbrest/touch",
                                       &thumbrestPath[Side::RIGHT]));

            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/system/click", &systemPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/system/click", &systemPath[Side::RIGHT]));

            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/back/click", &backPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/back/click", &backPath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/battery/value", &batteryPath[Side::LEFT]));
            CHECK_XRCMD(
                    xrStringToPath(xr_instance_, "/user/hand/right/input/battery/value", &batteryPath[Side::RIGHT]));

            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/trackpad/click",
                                       &TrackPadClickPath[Side::LEFT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/right/input/trackpad/click",
                                       &TrackPadClickPath[Side::RIGHT]));
            CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/hand/left/input/trackpad", &TrackPadValuePath[Side::LEFT]));
            CHECK_XRCMD(
                    xrStringToPath(xr_instance_, "/user/hand/right/input/trackpad", &TrackPadValuePath[Side::RIGHT]));

            // Suggest bindings for KHR Simple.
            {
                XrPath khrSimpleInteractionProfilePath;
                CHECK_XRCMD(xrStringToPath(xr_instance_, "/interaction_profiles/khr/simple_controller",
                                           &khrSimpleInteractionProfilePath));
                std::vector<XrActionSuggestedBinding> bindings{{// Fall back to a click input for the grab action.
                                                                {input_.grab_action, selectPath[Side::LEFT]},
                                                                {input_.grab_action, selectPath[Side::RIGHT]},
                                                                {input_.aim_pose_action, aimPosePath[Side::LEFT]},
                                                                {input_.aim_pose_action, aimPosePath[Side::RIGHT]},
                                                                {input_.grip_pose_action, gripPosePath[Side::LEFT]},
                                                                {input_.grip_pose_action, gripPosePath[Side::RIGHT]},
                                                                {input_.quit_action, menuClickPath[Side::LEFT]},
                                                                {input_.quit_action, menuClickPath[Side::RIGHT]},
                                                                {input_.vibrate_action, hapticPath[Side::LEFT]},
                                                                {input_.vibrate_action, hapticPath[Side::RIGHT]}}};
                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggestedBindings.interactionProfile = khrSimpleInteractionProfilePath;
                suggestedBindings.suggestedBindings = bindings.data();
                suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
                CHECK_XRCMD(xrSuggestInteractionProfileBindings(xr_instance_, &suggestedBindings));
            }
            // Suggest bindings for the Oculus Touch.
            {
                XrPath oculusTouchInteractionProfilePath;
                CHECK_XRCMD(xrStringToPath(xr_instance_, "/interaction_profiles/oculus/touch_controller",
                                           &oculusTouchInteractionProfilePath));
                std::vector<XrActionSuggestedBinding> bindings{
                        {{input_.grab_action, squeezeValuePath[Side::LEFT]},
                         {input_.grab_action, squeezeValuePath[Side::RIGHT]},

                         {input_.touchpad_action, thumbstickClickPath[Side::LEFT]},
                         {input_.touchpad_action, thumbstickClickPath[Side::RIGHT]},
                         {input_.joystick_action, thumbstickPosPath[Side::LEFT]},
                         {input_.joystick_action, thumbstickPosPath[Side::RIGHT]},
                         {input_.rocker_touch_action, thumbstickTouchPath[Side::LEFT]},
                         {input_.rocker_touch_action, thumbstickTouchPath[Side::RIGHT]},

                         {input_.thumb_rest_touch_action, thumbrestPath[Side::LEFT]},
                         {input_.thumb_rest_touch_action, thumbrestPath[Side::RIGHT]},

                         {input_.trigger_action, triggerValuePath[Side::LEFT]},
                         {input_.trigger_action, triggerValuePath[Side::RIGHT]},
                         {input_.trigger_touch_action, triggerTouchPath[Side::LEFT]},
                         {input_.trigger_touch_action, triggerTouchPath[Side::RIGHT]},
                         {input_.grip_action, squeezeValuePath[Side::LEFT]},
                         {input_.grip_action, squeezeValuePath[Side::RIGHT]},
                         {input_.aim_pose_action, aimPosePath[Side::LEFT]},
                         {input_.aim_pose_action, aimPosePath[Side::RIGHT]},
                         {input_.grip_pose_action, gripPosePath[Side::LEFT]},
                         {input_.grip_pose_action, gripPosePath[Side::RIGHT]},

                         {input_.home_action, systemPath[Side::RIGHT]},
                         {input_.back_action, menuClickPath[Side::LEFT]},
                         {input_.X_touch_action, XTouchPath[Side::LEFT]},
                         {input_.Y_touch_action, YTouchPath[Side::LEFT]},
                         {input_.A_touch_action, ATouchPath[Side::RIGHT]},
                         {input_.B_touch_action, BTouchPath[Side::RIGHT]},
                         {input_.X_action, XValuePath[Side::LEFT]},
                         {input_.Y_action, YValuePath[Side::LEFT]},
                         {input_.A_action, AValuePath[Side::RIGHT]},
                         {input_.B_action, BValuePath[Side::RIGHT]},

                         {input_.vibrate_action, hapticPath[Side::LEFT]},
                         {input_.vibrate_action, hapticPath[Side::RIGHT]}}};

                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggestedBindings.interactionProfile = oculusTouchInteractionProfilePath;
                suggestedBindings.suggestedBindings = bindings.data();
                suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
                CHECK_XRCMD(xrSuggestInteractionProfileBindings(xr_instance_, &suggestedBindings));
            }

            bool isSupportBDControllerInteraction = IsExtensionSupported(XR_BD_CONTROLLER_INTERACTION_EXTENSION_NAME) &&
                                                    IsExtensionEnabled(XR_BD_CONTROLLER_INTERACTION_EXTENSION_NAME);
            // Suggest bindings for PICO 4 Controller.&&controller_type_==XR_CV3_Phoenix_Controller_Type
            if (isSupportBDControllerInteraction) {
                XrPath picoMixedRealityInteractionProfilePath;

                CHECK_XRCMD(xrStringToPath(xr_instance_, "/interaction_profiles/bytedance/pico4_controller",
                                           &picoMixedRealityInteractionProfilePath));
                std::vector<XrActionSuggestedBinding> bindings{
                        {{input_.touchpad_action, thumbstickClickPath[Side::LEFT]},
                         {input_.touchpad_action, thumbstickClickPath[Side::RIGHT]},
                         {input_.joystick_action, thumbstickPosPath[Side::LEFT]},
                         {input_.joystick_action, thumbstickPosPath[Side::RIGHT]},
                         {input_.rocker_touch_action, thumbstickTouchPath[Side::LEFT]},
                         {input_.rocker_touch_action, thumbstickTouchPath[Side::RIGHT]},

                         {input_.thumb_rest_touch_action, thumbrestPath[Side::LEFT]},
                         {input_.thumb_rest_touch_action, thumbrestPath[Side::RIGHT]},

                         {input_.trigger_click_action, triggerClickPath[Side::LEFT]},
                         {input_.trigger_click_action, triggerClickPath[Side::RIGHT]},
                         {input_.trigger_action, triggerValuePath[Side::LEFT]},
                         {input_.trigger_action, triggerValuePath[Side::RIGHT]},
                         {input_.trigger_touch_action, triggerTouchPath[Side::LEFT]},
                         {input_.trigger_touch_action, triggerTouchPath[Side::RIGHT]},

                         {input_.side_action, squeezeClickPath[Side::LEFT]},
                         {input_.side_action, squeezeClickPath[Side::RIGHT]},
                         {input_.grip_action, squeezeValuePath[Side::LEFT]},
                         {input_.grip_action, squeezeValuePath[Side::RIGHT]},
                         {input_.aim_pose_action, aimPosePath[Side::LEFT]},
                         {input_.aim_pose_action, aimPosePath[Side::RIGHT]},
                         {input_.grip_pose_action, gripPosePath[Side::LEFT]},
                         {input_.grip_pose_action, gripPosePath[Side::RIGHT]},

                         {input_.home_action, systemPath[Side::LEFT]},
                         {input_.home_action, systemPath[Side::RIGHT]},
                         {input_.back_action, menuClickPath[Side::LEFT]},
                         {input_.back_action, menuClickPath[Side::RIGHT]},
                         {input_.battery_action, batteryPath[Side::LEFT]},
                         {input_.battery_action, batteryPath[Side::RIGHT]},

                         {input_.X_touch_action, XTouchPath[Side::LEFT]},
                         {input_.Y_touch_action, YTouchPath[Side::LEFT]},
                         {input_.A_touch_action, ATouchPath[Side::RIGHT]},
                         {input_.B_touch_action, BTouchPath[Side::RIGHT]},
                         {input_.X_action, XValuePath[Side::LEFT]},
                         {input_.Y_action, YValuePath[Side::LEFT]},
                         {input_.A_action, AValuePath[Side::RIGHT]},
                         {input_.B_action, BValuePath[Side::RIGHT]},
                         {input_.vibrate_action, hapticPath[Side::LEFT]},
                         {input_.vibrate_action, hapticPath[Side::RIGHT]}}};

                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggestedBindings.interactionProfile = picoMixedRealityInteractionProfilePath;
                suggestedBindings.suggestedBindings = bindings.data();
                suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
                (xrSuggestInteractionProfileBindings(xr_instance_, &suggestedBindings));
            }

            // Suggest bindings for PICO 4s / Hawk Controller.&&controller_type_==XR_CV3_Hawk_Controller_Type
            if (isSupportBDControllerInteraction) {
                XrPath picoMixedRealityInteractionProfilePath;

                CHECK_XRCMD(xrStringToPath(xr_instance_, "/interaction_profiles/bytedance/pico4s_controller",
                                           &picoMixedRealityInteractionProfilePath));
                std::vector<XrActionSuggestedBinding> bindings{
                        {{input_.touchpad_action, thumbstickClickPath[Side::LEFT]},
                         {input_.touchpad_action, thumbstickClickPath[Side::RIGHT]},
                         {input_.joystick_action, thumbstickPosPath[Side::LEFT]},
                         {input_.joystick_action, thumbstickPosPath[Side::RIGHT]},
                         {input_.rocker_touch_action, thumbstickTouchPath[Side::LEFT]},
                         {input_.rocker_touch_action, thumbstickTouchPath[Side::RIGHT]},

                         {input_.thumb_rest_touch_action, thumbrestPath[Side::LEFT]},
                         {input_.thumb_rest_touch_action, thumbrestPath[Side::RIGHT]},

                         {input_.trigger_click_action, triggerClickPath[Side::LEFT]},
                         {input_.trigger_click_action, triggerClickPath[Side::RIGHT]},
                         {input_.trigger_action, triggerValuePath[Side::LEFT]},
                         {input_.trigger_action, triggerValuePath[Side::RIGHT]},
                         {input_.trigger_touch_action, triggerTouchPath[Side::LEFT]},
                         {input_.trigger_touch_action, triggerTouchPath[Side::RIGHT]},

                         {input_.side_action, squeezeClickPath[Side::LEFT]},
                         {input_.side_action, squeezeClickPath[Side::RIGHT]},
                         {input_.grip_action, squeezeValuePath[Side::LEFT]},
                         {input_.grip_action, squeezeValuePath[Side::RIGHT]},
                         {input_.aim_pose_action, aimPosePath[Side::LEFT]},
                         {input_.aim_pose_action, aimPosePath[Side::RIGHT]},
                         {input_.grip_pose_action, gripPosePath[Side::LEFT]},
                         {input_.grip_pose_action, gripPosePath[Side::RIGHT]},

                         {input_.home_action, systemPath[Side::LEFT]},
                         {input_.home_action, systemPath[Side::RIGHT]},
                         {input_.back_action, menuClickPath[Side::LEFT]},
                         {input_.back_action, menuClickPath[Side::RIGHT]},
                         {input_.battery_action, batteryPath[Side::LEFT]},
                         {input_.battery_action, batteryPath[Side::RIGHT]},

                         {input_.X_touch_action, XTouchPath[Side::LEFT]},
                         {input_.Y_touch_action, YTouchPath[Side::LEFT]},
                         {input_.A_touch_action, ATouchPath[Side::RIGHT]},
                         {input_.B_touch_action, BTouchPath[Side::RIGHT]},
                         {input_.X_action, XValuePath[Side::LEFT]},
                         {input_.Y_action, YValuePath[Side::LEFT]},
                         {input_.A_action, AValuePath[Side::RIGHT]},
                         {input_.B_action, BValuePath[Side::RIGHT]},
                         {input_.vibrate_action, hapticPath[Side::LEFT]},
                         {input_.vibrate_action, hapticPath[Side::RIGHT]}}};

                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggestedBindings.interactionProfile = picoMixedRealityInteractionProfilePath;
                suggestedBindings.suggestedBindings = bindings.data();
                suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
                (xrSuggestInteractionProfileBindings(xr_instance_, &suggestedBindings));
            }

            // Suggest bindings for PICO NEO3 Controller.&&controller_type_==XR_CV3_Optics_Controller_Type
            if (isSupportBDControllerInteraction) {
                XrPath picoMixedRealityInteractionProfilePath;

                CHECK_XRCMD(xrStringToPath(xr_instance_, "/interaction_profiles/bytedance/pico_neo3_controller",
                                           &picoMixedRealityInteractionProfilePath));
                std::vector<XrActionSuggestedBinding> bindings{
                        {{input_.touchpad_action, thumbstickClickPath[Side::LEFT]},
                         {input_.touchpad_action, thumbstickClickPath[Side::RIGHT]},
                         {input_.joystick_action, thumbstickPosPath[Side::LEFT]},
                         {input_.joystick_action, thumbstickPosPath[Side::RIGHT]},
                         {input_.rocker_touch_action, thumbstickTouchPath[Side::LEFT]},
                         {input_.rocker_touch_action, thumbstickTouchPath[Side::RIGHT]},

                         {input_.thumb_rest_touch_action, thumbrestPath[Side::LEFT]},
                         {input_.thumb_rest_touch_action, thumbrestPath[Side::RIGHT]},

                         {input_.trigger_click_action, triggerClickPath[Side::LEFT]},
                         {input_.trigger_click_action, triggerClickPath[Side::RIGHT]},
                         {input_.trigger_action, triggerValuePath[Side::LEFT]},
                         {input_.trigger_action, triggerValuePath[Side::RIGHT]},
                         {input_.trigger_touch_action, triggerTouchPath[Side::LEFT]},
                         {input_.trigger_touch_action, triggerTouchPath[Side::RIGHT]},

                         {input_.side_action, squeezeClickPath[Side::LEFT]},
                         {input_.side_action, squeezeClickPath[Side::RIGHT]},
                         {input_.grip_action, squeezeValuePath[Side::LEFT]},
                         {input_.grip_action, squeezeValuePath[Side::RIGHT]},
                         {input_.aim_pose_action, aimPosePath[Side::LEFT]},
                         {input_.aim_pose_action, aimPosePath[Side::RIGHT]},
                         {input_.grip_pose_action, gripPosePath[Side::LEFT]},
                         {input_.grip_pose_action, gripPosePath[Side::RIGHT]},

                         {input_.home_action, systemPath[Side::LEFT]},
                         {input_.home_action, systemPath[Side::RIGHT]},
                         {input_.back_action, menuClickPath[Side::LEFT]},
                         {input_.back_action, menuClickPath[Side::RIGHT]},
                         {input_.battery_action, batteryPath[Side::LEFT]},
                         {input_.battery_action, batteryPath[Side::RIGHT]},

                         {input_.X_touch_action, XTouchPath[Side::LEFT]},
                         {input_.Y_touch_action, YTouchPath[Side::LEFT]},
                         {input_.A_touch_action, ATouchPath[Side::RIGHT]},
                         {input_.B_touch_action, BTouchPath[Side::RIGHT]},
                         {input_.X_action, XValuePath[Side::LEFT]},
                         {input_.Y_action, YValuePath[Side::LEFT]},
                         {input_.A_action, AValuePath[Side::RIGHT]},
                         {input_.B_action, BValuePath[Side::RIGHT]},
                         {input_.vibrate_action, hapticPath[Side::LEFT]},
                         {input_.vibrate_action, hapticPath[Side::RIGHT]}}};

                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggestedBindings.interactionProfile = picoMixedRealityInteractionProfilePath;
                suggestedBindings.suggestedBindings = bindings.data();
                suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
                (xrSuggestInteractionProfileBindings(xr_instance_, &suggestedBindings));
            }

            // Suggest bindings for PICO G3 Controller.&&controller_type_==XR_CV3_MerlinE_Controller_Type
            if (isSupportBDControllerInteraction) {
                XrPath picoMixedRealityInteractionProfilePath;

                CHECK_XRCMD(xrStringToPath(xr_instance_, "/interaction_profiles/bytedance/pico_g3_controller",
                                           &picoMixedRealityInteractionProfilePath));
                std::vector<XrActionSuggestedBinding> bindings{{
                        {input_.touchpad_action, thumbstickClickPath[Side::LEFT]},
                        {input_.touchpad_action, thumbstickClickPath[Side::RIGHT]},
                        {input_.joystick_action, thumbstickPosPath[Side::LEFT]},
                        {input_.joystick_action, thumbstickPosPath[Side::RIGHT]},
                        {input_.trigger_click_action, triggerClickPath[Side::LEFT]},
                        {input_.trigger_click_action, triggerClickPath[Side::RIGHT]},
                        {input_.trigger_action, triggerValuePath[Side::LEFT]},
                        {input_.trigger_action, triggerValuePath[Side::RIGHT]},
                        {input_.aim_pose_action, aimPosePath[Side::LEFT]},
                        {input_.aim_pose_action, aimPosePath[Side::RIGHT]},
                        {input_.grip_pose_action, gripPosePath[Side::LEFT]},
                        {input_.grip_pose_action, gripPosePath[Side::RIGHT]},
                        {input_.home_action, systemPath[Side::LEFT]},
                        {input_.home_action, systemPath[Side::RIGHT]},
                        {input_.back_action, menuClickPath[Side::LEFT]},
                        {input_.back_action, menuClickPath[Side::RIGHT]},
                        {input_.battery_action, batteryPath[Side::LEFT]},
                        {input_.battery_action, batteryPath[Side::RIGHT]},

                        {input_.trackpad_click_action, TrackPadClickPath[Side::LEFT]},
                        {input_.trackpad_click_action, TrackPadClickPath[Side::RIGHT]},
                        {input_.trackpad_value_action, TrackPadValuePath[Side::LEFT]},
                        {input_.trackpad_value_action, TrackPadValuePath[Side::RIGHT]},

                }};

                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggestedBindings.interactionProfile = picoMixedRealityInteractionProfilePath;
                suggestedBindings.suggestedBindings = bindings.data();
                suggestedBindings.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
                (xrSuggestInteractionProfileBindings(xr_instance_, &suggestedBindings));
            }

            // eye tracking
            if (eye_tracking_supported_) {
                XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
                strcpy_s(actionInfo.actionName, "openxreyetrackeraction");
                actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
                strcpy_s(actionInfo.localizedActionName, "EyeTrackerAction");
                CHECK_XRCMD(xrCreateAction(input_.action_set, &actionInfo, &input_.gaze_action));

                XrPath eyeGazeInteractionProfilePath;
                XrPath gazePosePath;
                CHECK_XRCMD(xrStringToPath(xr_instance_, "/interaction_profiles/ext/eye_gaze_interaction",
                                           &eyeGazeInteractionProfilePath));
                CHECK_XRCMD(xrStringToPath(xr_instance_, "/user/eyes_ext/input/gaze_ext/pose", &gazePosePath));

                XrActionSuggestedBinding bindings;
                bindings.action = input_.gaze_action;
                bindings.binding = gazePosePath;

                XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
                suggestedBindings.interactionProfile = eyeGazeInteractionProfilePath;
                suggestedBindings.suggestedBindings = &bindings;
                suggestedBindings.countSuggestedBindings = 1;
                CHECK_XRCMD(xrSuggestInteractionProfileBindings(xr_instance_, &suggestedBindings));

                XrActionSpaceCreateInfo createActionSpaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
                createActionSpaceInfo.action = input_.gaze_action;
                XrPosef t{};
                t.orientation.w = 1;
                createActionSpaceInfo.poseInActionSpace = t;
                CHECK_XRCMD(xrCreateActionSpace(xr_session_, &createActionSpaceInfo, &input_.gaze_action_space));
            }

            // create hand space (grip space)
            XrActionSpaceCreateInfo actionSpaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
            actionSpaceInfo.action = input_.grip_pose_action;
            actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
            actionSpaceInfo.subactionPath = input_.controller_subaction_paths[Side::LEFT];
            CHECK_XRCMD(xrCreateActionSpace(xr_session_, &actionSpaceInfo, &input_.controller_grip_spaces[Side::LEFT]));
            actionSpaceInfo.subactionPath = input_.controller_subaction_paths[Side::RIGHT];
            CHECK_XRCMD(
                    xrCreateActionSpace(xr_session_, &actionSpaceInfo, &input_.controller_grip_spaces[Side::RIGHT]));

            // create aim space
            actionSpaceInfo.action = input_.aim_pose_action;
            actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
            actionSpaceInfo.subactionPath = input_.controller_subaction_paths[Side::LEFT];
            CHECK_XRCMD(xrCreateActionSpace(xr_session_, &actionSpaceInfo, &input_.controller_aim_spaces[Side::LEFT]));
            actionSpaceInfo.subactionPath = input_.controller_subaction_paths[Side::RIGHT];
            CHECK_XRCMD(xrCreateActionSpace(xr_session_, &actionSpaceInfo, &input_.controller_aim_spaces[Side::RIGHT]));

            XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
            attachInfo.next = nullptr;
            attachInfo.countActionSets = 1;
            attachInfo.actionSets = &input_.action_set;

            CHECK_XRCMD(xrAttachSessionActionSets(xr_session_, &attachInfo));
        }

        /// extension features cb
        auto extensions = extension_features_manager_->GetAllRegisteredExtensions();
        for (auto& feature : extensions) {
            feature->OnActionsInit();
        }

        // Customized initialize actions
        if (!CustomizedActionsInit()) {
            PLOGE("CustomizedActionsInit failed");
            THROW_XR(XR_ERROR_RUNTIME_FAILURE, "CustomizedActionsInit failed");
        }

        PLOGI("InitializeActions End.\n");
        return true;
    }

    bool BasicOpenXrWrapper::IsExtensionSupported(const char* extensionName) const {
        for (XrExtensionProperties extensionProperties : runtime_supported_extensions_) {
            if (strcmp(extensionName, extensionProperties.extensionName) == 0) {
                PLOGI("Runtime Support: %s", extensionName);
                return true;
            }
        }
        PLOGI("Runtime not Support: %s", extensionName);
        return false;
    }

    bool BasicOpenXrWrapper::IsExtensionEnabled(const char* extensionName) const {
        for (auto extension : enabled_extensions_) {
            if (strcmp(extensionName, extension.c_str()) == 0) {
                PLOGI("App Enabled extension: %s", extensionName);
                return true;
            }
        }
        PLOGI("Runtime not Enabled: %s", extensionName);
        return false;
    }

    int BasicOpenXrWrapper::GetControllerType() {
        XrInteractionProfileState Profile;
        Profile.type = XR_TYPE_INTERACTION_PROFILE_STATE;
        Profile.next = nullptr;
        auto ret = xrGetCurrentInteractionProfile(xr_session_, input_.controller_subaction_paths[Side::LEFT], &Profile);
        char Path[128];
        if (Profile.interactionProfile != XR_NULL_PATH) {
            uint32_t PathCount;
            xrPathToString(xr_instance_, Profile.interactionProfile, 128, &PathCount, Path);
        }
        PLOGI("GetControllerType xrGetCurrentInteractionProfile ret = %d Path  %s", ret, Path);

        if (strcmp(Path, "/interaction_profiles/bytedance/pico_neo3_controller") == 0) {
            controller_type_ = XR_CV3_Optics_Controller_Type;
        } else if (strcmp(Path, "/interaction_profiles/bytedance/pico4_controller") == 0) {
            controller_type_ = XR_CV3_Phoenix_Controller_Type;
        } else if (strcmp(Path, "/interaction_profiles/bytedance/pico4s_controller") == 0) {
            controller_type_ = XR_CV3_Hawk_Controller_Type;
        } else if (strcmp(Path, "/interaction_profiles/bytedance/pico_g3_controller") == 0) {
            controller_type_ = XR_CV3_MerlinE_Controller_Type;
        } else {
            controller_type_ = Simple_Controller;
        }
        return controller_type_;
    }

    void BasicOpenXrWrapper::RegisterXrEventHandler(XrEventHandler handler) {
        registered_event_handlers_.push_back(handler);
    }

    bool BasicOpenXrWrapper::RegisterExtensionFeature(const std::shared_ptr<IOpenXRExtensionPlugin> modularFeature) {
        if (nullptr == modularFeature) {
            PLOGE("BasicOpenXrWrapper::RegisterExtensionFeature skip, modularFeature is nullptr");
            return false;
        }

        if (runtime_supported_extensions_.empty()) {
            PLOGI("RegisterExtensionFeature runtime_supported_extensions_ is empty,"
                  "get runtime_supported_extensions_ first");
            GetLayersAndExtensions();
        }

        auto needExtensions = modularFeature->GetRequiredExtensions();
        for (auto& extension : needExtensions) {
            if (!IsExtensionSupported(extension.c_str())) {
                PLOGE("BasicOpenXrWrapper::RegisterExtensionFeature skip, extension %s not supported",
                      extension.c_str());
                throw ExtensionNotSupportedException(extension);
                return false;
            }
        }
        extension_features_manager_->RegisterExtensionFeature(modularFeature);
        return true;
    }

    void BasicOpenXrWrapper::CustomizedExtensionAndFeaturesInit() {
        /// extension features init
        /// TODO:

        /// Other features init
        /// TODO:
    }

    BasicOpenXrWrapper::~BasicOpenXrWrapper() {
        // release vectors
        if (!registered_event_handlers_.empty()) {
            std::vector<XrEventHandler>().swap(registered_event_handlers_);
        }
        if (!runtime_supported_extensions_.empty()) {
            std::vector<XrExtensionProperties>().swap(runtime_supported_extensions_);
        }
        if (!enabled_extensions_.empty()) {
            std::vector<std::string>().swap(enabled_extensions_);
        }
        if (!config_views_.empty()) {
            std::vector<XrViewConfigurationView>().swap(config_views_);
        }
        if (!projection_views_cache_.empty()) {
            std::vector<XrView>().swap(projection_views_cache_);
        }

        // release xr objects
        if (xr_session_ != XR_NULL_HANDLE) {
            xrDestroySession(xr_session_);
            xr_session_ = XR_NULL_HANDLE;
        }
        if (xr_system_id_ != XR_NULL_SYSTEM_ID) {
            xr_system_id_ = XR_NULL_SYSTEM_ID;
        }
        if (xr_instance_ != XR_NULL_HANDLE) {
            xrDestroyInstance(xr_instance_);
            xr_instance_ = XR_NULL_HANDLE;
        }
    }

    void BasicOpenXrWrapper::GetSystemProperties() {
        // Read graphics properties for preferred swapchain length and logging.
        CHECK_XRCMD(xrGetSystemProperties(xr_instance_, xr_system_id_, &system_properties_));

        // Log system properties.
        PLOGI("System Properties: Name=%s VendorId=%d", system_properties_.systemName, system_properties_.vendorId);
        PLOGI("System Graphics Properties: MaxWidth=%d MaxHeight=%d MaxLayers=%d",
              system_properties_.graphicsProperties.maxSwapchainImageWidth,
              system_properties_.graphicsProperties.maxSwapchainImageHeight,
              system_properties_.graphicsProperties.maxLayerCount);
        PLOGI("System Tracking Properties: OrientationTracking=%s PositionTracking=%s",
              system_properties_.trackingProperties.orientationTracking == XR_TRUE ? "True" : "False",
              system_properties_.trackingProperties.positionTracking == XR_TRUE ? "True" : "False");
    }
}  // namespace PVRSampleFW
