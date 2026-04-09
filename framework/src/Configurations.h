// Copyright (c) 2017-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef PICONATIVEOPENXRSAMPLES_CONFIGURATIONS_H
#define PICONATIVEOPENXRSAMPLES_CONFIGURATIONS_H

#include "util/Common.h"

namespace PVRSampleFW {
    inline XrFormFactor GetXrFormFactor(const std::string& formFactorStr) {
        if (EqualsIgnoreCase(formFactorStr, "Hmd")) {
            return XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        }
        if (EqualsIgnoreCase(formFactorStr, "Handheld")) {
            return XR_FORM_FACTOR_HANDHELD_DISPLAY;
        }
        throw std::invalid_argument(Fmt("Unknown form factor '%s'", formFactorStr.c_str()));
    }

    inline XrViewConfigurationType GetXrViewConfigurationType(const std::string& viewConfigurationStr) {
        if (EqualsIgnoreCase(viewConfigurationStr, "Mono")) {
            return XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
        }
        if (EqualsIgnoreCase(viewConfigurationStr, "Stereo")) {
            return XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        }
        throw std::invalid_argument(Fmt("Unknown view configuration '%s'", viewConfigurationStr.c_str()));
    }

    inline XrEnvironmentBlendMode GetXrEnvironmentBlendMode(const std::string& environmentBlendModeStr) {
        if (EqualsIgnoreCase(environmentBlendModeStr, "Opaque")) {
            return XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        }
        if (EqualsIgnoreCase(environmentBlendModeStr, "Additive")) {
            return XR_ENVIRONMENT_BLEND_MODE_ADDITIVE;
        }
        if (EqualsIgnoreCase(environmentBlendModeStr, "AlphaBlend")) {
            return XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND;
        }
        throw std::invalid_argument(Fmt("Unknown environment blend mode '%s'", environmentBlendModeStr.c_str()));
    }

    inline XrReferenceSpaceType GetXrSpace(const std::string& appSpaceStr) {
        if (EqualsIgnoreCase(appSpaceStr, "Local")) {
            return XR_REFERENCE_SPACE_TYPE_LOCAL;
        }

        if (EqualsIgnoreCase(appSpaceStr, "Stage")) {
            return XR_REFERENCE_SPACE_TYPE_STAGE;
        }

        if (EqualsIgnoreCase(appSpaceStr, "View")) {
            return XR_REFERENCE_SPACE_TYPE_VIEW;
        }

        if (EqualsIgnoreCase(appSpaceStr, "LocalFloor")) {
            return XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR;
        }

        if (EqualsIgnoreCase(appSpaceStr, "UnboundedMsft")) {
            return XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT;
        }

        if (EqualsIgnoreCase(appSpaceStr, "CombinedEyeVarjo")) {
            return XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO;
        }

        if (EqualsIgnoreCase(appSpaceStr, "LocalizationMapML")) {
            return XR_REFERENCE_SPACE_TYPE_LOCALIZATION_MAP_ML;
        }

        if (EqualsIgnoreCase(appSpaceStr, "LocalFloorExt")) {
            return XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT;
        }

        throw std::invalid_argument(Fmt("Unknown app space '%s'", appSpaceStr.c_str()));
    }

    inline const char* GetXrEnvironmentBlendModeStr(XrEnvironmentBlendMode environmentBlendMode) {
        switch (environmentBlendMode) {
        case XR_ENVIRONMENT_BLEND_MODE_OPAQUE:
            return "Opaque";
        case XR_ENVIRONMENT_BLEND_MODE_ADDITIVE:
            return "Additive";
        case XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND:
            return "AlphaBlend";
        default:
            throw std::invalid_argument(Fmt("Unknown environment blend mode '%s'", to_string(environmentBlendMode)));
        }
    }

    struct Configurations {
        std::string graphics_plugin{"OpenGLES"};

        std::string platform_plugin{"Android"};

        std::string form_factor{"Hmd"};

        std::string view_configuration{"Stereo"};

        std::string environment_blend_mode{"Opaque"};

        std::string app_space_type{"Local"};

        std::string target_refresh_rate{"90"};

        int perf_setting_cpu_level{2};
        int perf_setting_gpu_level{3};
        bool use_input_handling{true};

        struct ConfigParsed {
            XrFormFactor formfactor{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};

            XrViewConfigurationType viewconfigtype{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};

            XrEnvironmentBlendMode environmentblendmode{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};

            XrReferenceSpaceType appspace{XR_REFERENCE_SPACE_TYPE_LOCAL};

            float targetrefreshrate{90.0f};
        } parsed;

        void ParseString() {
            parsed.formfactor = GetXrFormFactor(form_factor);
            parsed.viewconfigtype = GetXrViewConfigurationType(view_configuration);
            parsed.environmentblendmode = GetXrEnvironmentBlendMode(environment_blend_mode);
            parsed.appspace = GetXrSpace(app_space_type);
            parsed.targetrefreshrate = std::stof(target_refresh_rate);
        }

        void SetEnvironmentBlendMode(XrEnvironmentBlendMode envBlm) {
            environment_blend_mode = GetXrEnvironmentBlendModeStr(envBlm);
            parsed.environmentblendmode = envBlm;
        }

        std::array<float, 4> GetBackgroundClearColor() const {
            static const std::array<float, 4> SlateGrey{0.184313729f, 0.309803933f, 0.309803933f, 1.0f};
            static const std::array<float, 4> TransparentBlack{0.0f, 0.0f, 0.0f, 0.0f};
            static const std::array<float, 4> Black{0.0f, 0.0f, 0.0f, 1.0f};

            switch (parsed.environmentblendmode) {
            case XR_ENVIRONMENT_BLEND_MODE_OPAQUE:
                return SlateGrey;
            case XR_ENVIRONMENT_BLEND_MODE_ADDITIVE:
                return Black;
            case XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND:
                return TransparentBlack;
            default:
                return SlateGrey;
            }
        }
    };
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_CONFIGURATIONS_H
