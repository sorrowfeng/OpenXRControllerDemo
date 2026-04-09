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

#ifndef PICONATIVEOPENXRSAMPLES_PICOBODYTRACKING_H
#define PICONATIVEOPENXRSAMPLES_PICOBODYTRACKING_H
#include "IXrPlatformPlugin.h"
#include "openxr/openxr.h"
#include "IOpenXRExtensionPlugin.h"
#include "CheckUtils.h"

namespace PVRSampleFW {

    class PICOBodyTracking : public IOpenXRExtensionPlugin {
    public:
        virtual ~PICOBodyTracking() = default;

        PICOBodyTracking() = default;
        PICOBodyTracking(const std::shared_ptr<IXrPlatformPlugin>& plaf, BasicOpenXrWrapper* openxrW)
            : IOpenXRExtensionPlugin(plaf, openxrW) {
        }

        std::vector<std::string> GetRequiredExtensions() const override;

        bool OnInstanceCreate() override;

        bool OnSystemGet(XrSystemProperties* configProperties) override;

        // int CreateBodyTracker(const XrBodyTrackerCreateInfoBD* createInfo, XrBodyTrackerBD* bodyTracker);

        // int LocateBodyJoints(XrBodyTrackerBD bodyTracker, const XrBodyJointsLocateInfoBD* locateInfo,
        //                      XrBodyJointLocationsBD* locations);

        // int DestroyBodyTracker(XrBodyTrackerBD bodyTracker);

        int xrSetBodyBoneLength(XrBodyBoneLengthPICO bonelength);

        int xrGetBodyTrackingPostureFlag(XrBodyTrackingPostureFlagsDataPICO* postureflags);

        int xrStartBodyTrackingCalibrationApp();

        int xrGetBodyTrackingState(XrBodyTrackingStatePICO* state);

    public:
        PFN_DECLARE(xrCreateBodyTrackerBD);
        PFN_DECLARE(xrDestroyBodyTrackerBD);
        PFN_DECLARE(xrLocateBodyJointsBD);
        PFN_DECLARE(xrStartBodyTrackingCalibrationAppPICO);
        PFN_DECLARE(xrGetBodyTrackingStatePICO);

        XrSystemBodyTrackingPropertiesBD body_tracking_system_properties_;
        XrBodyTrackerBD body_tracker_ = XR_NULL_HANDLE;
        XrBodyJointLocationBD body_joint_location_[XR_BODY_JOINT_COUNT_BD];
        XrBodyTrackingPosturePICO flags_[XR_BODY_JOINT_COUNT_BD];
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_PICOBODYTRACKING_H
