/*
 * Copyright 2025 - 2025 PICO. All rights reserved.  
 *
 * NOTICE: All information contained herein is, and remains the property of PICO.
 * The intellectual and technical concepts contained herein are proprietary to PICO.
 * and may be covered by patents, patents in process, and are protected by trade
 * secret or copyright law. Dissemination of this information or reproduction of
 * this material is strictly forbidden unless prior written permission is obtained
 * from PICO.
 */

#include "ExceptionHandlerProgram.h"
#include "Cube.h"
#include "GuiPlane.h"

namespace PVRSampleFW {

#ifdef XR_USE_PLATFORM_ANDROID
    bool ExceptionHandlerProgram::CustomizedAppPostInit() {
        auto ret = AndroidOpenXrProgram::CustomizedAppPostInit();
        AddControllerCubes();
        AddExceptionPopWindow();
        return ret;
    }

    bool ExceptionHandlerProgram::CustomizedXrInputHandlerSetup() {
        AndroidOpenXrProgram::CustomizedXrInputHandlerSetup();

        // register the input callback to the openxr wrapper.
        auto handleInputFunc = [](class BasicOpenXrWrapper *openxr, const PVRSampleFW::XrFrameIn &frameIn) {
            auto pOpenXrAppWrapper = dynamic_cast<ExceptionHandlerProgram *>(openxr);
            for (int hand = 0; hand < Side::COUNT; hand++) {
                if (frameIn.controller_actives[hand]) {
                    auto triggerValue = frameIn.controller_trigger_value[hand];
                    // Scale the rendered hand by 1.0f (open) to 0.5f (fully squeezed).
                    auto input = pOpenXrAppWrapper->GetInputState();
                    auto scale = 1.0f - 0.5f * triggerValue;
                    pOpenXrAppWrapper->SetControllerScale(hand, scale);

                    // Apply a vibration feedback to the controller
                    if (triggerValue > 0.9f) {
                        XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
                        vibration.amplitude = 0.5;
                        vibration.duration = XR_MIN_HAPTIC_DURATION;
                        vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

                        XrHapticActionInfo hapticActionInfo{XR_TYPE_HAPTIC_ACTION_INFO};
                        hapticActionInfo.action = input.vibrate_action;
                        hapticActionInfo.subactionPath = input.controller_subaction_paths[hand];
                        auto xrSession = pOpenXrAppWrapper->GetXrSession();
                        CHECK_XRCMD(xrApplyHapticFeedback(xrSession, &hapticActionInfo,
                                                          reinterpret_cast<XrHapticBaseHeader *>(&vibration)));
                    }
                }
            }
        };
        RegisterHandleInputFunc(handleInputFunc);
        return true;
    }

    bool ExceptionHandlerProgram::CustomizedPreRenderFrame() {
        AndroidOpenXrProgram::CustomizedPreRenderFrame();

        UpdateControllers();
        return true;
    }

    void ExceptionHandlerProgram::UpdateControllers() {
        Scene &scene = scenes_.at(SAMPLE_SCENE_TYPE_CONTROLLER);
        auto Objects = scene.GetAllObjects();
        for (int i = 0; i < Side::COUNT; i++) {
            auto hand = scene.GetObject(controller_ids_[i]);
            if (current_frame_in_.controller_actives[i]) {
                XrPosef handPose = current_frame_in_.controller_poses[i];
                hand->SetPose(handPose);

                auto scale = input_.controller_scales[i] * 0.1f;
                XrVector3f handScale = {scale, scale, scale};
                hand->SetScale(handScale);
            }
        }
    }

    void ExceptionHandlerProgram::SetControllerScale(int hand, float scale) {
        input_.controller_scales[hand] = scale;
    }

    void ExceptionHandlerProgram::AddControllerCubes() {
        Scene &scene = scenes_.at(SAMPLE_SCENE_TYPE_CONTROLLER);
        // Add hand cubes
        for (int i = 0; i < Side::COUNT; i++) {
            XrPosef handPose = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};
            float scale = 0.1f;
            XrVector3f handScale = {scale, scale, scale};
            auto cubeHand = std::make_shared<PVRSampleFW::Cube>(handPose, handScale);
            controller_ids_[i] = scene.AddObject(cubeHand);
            PLOGI("Add hand cube at id %d", controller_ids_[i]);
        }
    }

    void ExceptionHandlerProgram::AddExceptionPopWindow() {
        GuiWindow::Builder builder;

        std::string synopsis = "Something went wrong in your application.\n";

        std::string solution;

        if (nullptr != exception_to_handle_) {
            switch (exception_to_handle_->ExceptionType()) {
            case XR_EXCEPTION_EXTENSION_NOT_SUPPORTED: {
                solution = "Your program uses OpenXR extensions that are not supported by the runtime!\n";
                solution += exception_to_handle_->what();
                solution += "\n";
                solution += "Please remove the enable of the extension and related API calls and try again.";
                break;
            }
            case XR_EXCEPTION_COMMON: {
                solution = "Something unexpected happened in your program!\n";
                solution += exception_to_handle_->what();
                solution += "\n";
                solution += "Please check your application and try again.";
                break;
            }
            default: {
                solution = "Unknown exception type\n";
                solution += exception_to_handle_->what();
                solution += "\n";
                solution += "Please check your application and try again.";
                break;
            }
            }
        }

        std::string text = synopsis + solution;

        std::shared_ptr<GuiWindow> window = builder.SetTitle("Exception Occurred!")
                                                    .SetSize(800, 600)
                                                    .SetBgColor(0.3f, 0.2f, 0.4f, 0.6f)
                                                    .SetText(text.c_str())
                                                    .SetFontSize(24)
                                                    .SetTextColor(0.1f, 0.8f, 0.1f, 1.0f)
                                                    .Build();
        window->AddButton("Confirm and quit the app", [&]() {
            PLOGI("Confirm and quit the app");
            exit(0);
        });

        Scene &guiScene = scenes_.at(SAMPLE_SCENE_TYPE_GUI);
        XrPosef guiPose = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.5f, 0.5f, -3.0f}};
        XrVector3f guiScale = {2.0f, 1.5f, 1.0f};
        auto guiPlane = std::make_shared<GuiPlane>(guiPose, guiScale, window);
        guiScene.AddObject(guiPlane);
    }

#endif  // end of XR_USE_PLATFORM_ANDROID
}  // namespace PVRSampleFW
