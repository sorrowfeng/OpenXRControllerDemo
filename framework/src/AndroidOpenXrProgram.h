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

#ifndef PICONATIVEOPENXRSAMPLES_ANDROIDOPENXRPROGRAM_H
#define PICONATIVEOPENXRSAMPLES_ANDROIDOPENXRPROGRAM_H

#include "IXrProgram.h"
#include "BasicOpenXrWrapper.h"
#include "ImGuiRenderer.h"
#include "ExceptionUtils.h"

namespace PVRSampleFW {
    class AndroidOpenXrProgram;
    class ActivityMainLoopContext {
    public:
        ActivityMainLoopContext(JNIEnv* envParam, jobject activityParam, JavaVM* javaVm, struct android_app* app,
                                AndroidOpenXrProgram* activityBasedProgramParam)
            : env_(envParam)
            , activity_(activityParam)
            , jvm_(javaVm)
            , android_app_(app)
            , activity_based_program_(activityBasedProgramParam) {
        }
        ~ActivityMainLoopContext() {
            if (jvm_ != nullptr) {
                jvm_->DetachCurrentThread();
            }
        }
        bool IsResumed() const;

        void HandleOsEvents(bool firstFrame = false);

        bool ShouldExitMainLoop() const;

    private:
        JNIEnv* env_{nullptr};
        jobject activity_{nullptr};
        JavaVM* jvm_{nullptr};
        struct android_app* android_app_{nullptr};
        AndroidOpenXrProgram* activity_based_program_{nullptr};
    };

    struct AndroidActivityState {
        ANativeWindow* native_window = nullptr;
        bool resumed = false;
    };

    class AndroidOpenXrProgram : public IXrProgram, public BasicOpenXrWrapper {
    public:
        AndroidOpenXrProgram() = default;

        explicit AndroidOpenXrProgram(const std::shared_ptr<PVRSampleFW::Configurations>& appConfigParam)
            : app_config_(appConfigParam) {
        }

        ~AndroidOpenXrProgram() = default;

#pragma region program
    public:
        void Initialize() override;

        void Shutdown() override;

        int PollEvents() override;

        /// @brief Run the program. The entry point of the program.
        ///
        /// @param[in] app The android app.
        void Run(struct android_app* app);

        /**
        * Process the next main command.
        */
        static void AppHandleCmd(struct android_app* app, int32_t cmd);

        /// @brief Get the internal data path.
        ///
        /// @return The internal data path.
        std::string GetInternalDataPath() const {
            return internal_data_path_;
        }

        /// @brief Get the external data path.
        ///
        /// @return The external data path.
        std::string GetExternalDataPath() const {
            return external_data_path_;
        }

        /// @brief Get the asset manager.
        ///
        /// @return The asset manager.
        AAssetManager* GetAssetManager() const {
            return asset_manager_;
        }

        /**
         * Load a file from the asset manager.
         *
         * @param filename The name of the file to load.
         * @return The file data.
         */
        std::vector<uint8_t> LoadFileFromAsset(const std::string& filename) const override;

        bool IsResumed() const;

    protected:
        /**
        * @brief Set up your owned game scenes here
        * @return
        */
        virtual bool CustomizedAppPostInit();

    private:
        /// @brief Initialize the android environment.
        void InitializeAndroidEnv(struct android_app* app);

        void UpdateAppConfig();

        void ShowSetAppConfigHelp();

        /// @brief Core Loop of the program.
        void Loop();

        void HandleCollisionDetection();

        void UpdateRay(const XrPosef& pose, const float& distance, const int& hand);

        /// @brief Render objects in the scenes paradigm
        void RenderScenes();

        void SwitchToExceptionHandlerProgram(const XrException& e);

        std::string GetApplicationName() override {
            return "AndroidOpenXrProgram";
        }
#pragma endregion

#pragma region openxr
    protected:
        void RenderFrame() override;

        void CustomizedExtensionAndFeaturesInit() override;
#pragma endregion

    protected:
        struct android_app* android_app_{nullptr};
        AndroidActivityState app_state_;
        std::unique_ptr<ActivityMainLoopContext> main_loop_context_{nullptr};
        std::shared_ptr<PVRSampleFW::Configurations> app_config_{nullptr};

        std::string internal_data_path_;
        std::string external_data_path_;
        AAssetManager* asset_manager_{nullptr};

        std::vector<PVRSampleFW::Scene> scenes_{SAMPLE_SCENE_TYPE_NUM};
        ICollisionDetector* collision_detector_{nullptr};
        int64_t ray_obj_id_[Side::COUNT]{-1, -1};
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_ANDROIDOPENXRPROGRAM_H
