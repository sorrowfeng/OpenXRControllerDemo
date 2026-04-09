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

#ifndef PICONATIVEOPENXRSAMPLES_EXCEPTIONHANDLERPROGRAM_H
#define PICONATIVEOPENXRSAMPLES_EXCEPTIONHANDLERPROGRAM_H

#include "AndroidOpenXrProgram.h"
#include "ExceptionUtils.h"

namespace PVRSampleFW {

#ifdef XR_USE_PLATFORM_ANDROID
    class ExceptionHandlerProgram : public AndroidOpenXrProgram {
    public:
        ExceptionHandlerProgram(const std::shared_ptr<PVRSampleFW::Configurations>& config, XrException* exception)
            : AndroidOpenXrProgram(config), exception_to_handle_(exception) {
        }
        ExceptionHandlerProgram() = delete;
        ~ExceptionHandlerProgram() = default;

        bool CustomizedAppPostInit() override;

        bool CustomizedXrInputHandlerSetup() override;

        bool CustomizedPreRenderFrame() override;

    private:
        void UpdateControllers();

        void SetControllerScale(int hand, float scale);

        void AddControllerCubes();

        void AddExceptionPopWindow();

    private:
        XrException* exception_to_handle_;
        int64_t controller_ids_[Side::COUNT] = {-1, -1};
    };
#endif

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_EXCEPTIONHANDLERPROGRAM_H
