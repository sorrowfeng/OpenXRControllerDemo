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

#include "PICOBodyTracking.h"
#include "BasicOpenXrWrapper.h"

namespace PVRSampleFW {
    std::vector<std::string> PICOBodyTracking::GetRequiredExtensions() const {
        return {XR_PICO_BODY_TRACKING2_EXTENSION_NAME};
    }

    bool PICOBodyTracking::OnInstanceCreate() {
        // Initialize the function pointers
        if (openxr_wrapper_ == nullptr) {
            PLOGE("BDBodyTracking::OnInstanceCreate failed for openxr_wrapper_ is null");
            return false;
        }

        auto is_pico_body_tracking_enabled = openxr_wrapper_->IsExtensionEnabled(XR_PICO_BODY_TRACKING2_EXTENSION_NAME);
        PLOGE("BDBodyTrackingInit::OnInstanceCreate is_pico_body_tracking2_enabled %d", is_pico_body_tracking_enabled);

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrCreateBodyTrackerBD",
                                          reinterpret_cast<PFN_xrVoidFunction *>(&xrCreateBodyTrackerBD)));

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrDestroyBodyTrackerBD",
                                          reinterpret_cast<PFN_xrVoidFunction *>(&xrDestroyBodyTrackerBD)));

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrLocateBodyJointsBD",
                                          reinterpret_cast<PFN_xrVoidFunction *>(&xrLocateBodyJointsBD)));

        CHECK_XRCMD(
                xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrStartBodyTrackingCalibrationAppPICO",
                                      reinterpret_cast<PFN_xrVoidFunction *>(&xrStartBodyTrackingCalibrationAppPICO)));

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrGetBodyTrackingStatePICO",
                                          reinterpret_cast<PFN_xrVoidFunction *>(&xrGetBodyTrackingStatePICO)));

        return true;
    }

    bool PICOBodyTracking::OnSystemGet(XrSystemProperties *configProperties) {
        configProperties->next = &body_tracking_system_properties_;
        return true;
    }

    /*int PICOBodyTracking::CreateBodyTracker(const XrBodyTrackerCreateInfoBD* createInfo,
            XrBodyTrackerBD* bodyTracker) {
        PLOGE("BDBodyTracking::CreateBodyTracker");
        CHECK_POINTER_ARG_IS_NOT_NULL(xrCreateBodyTrackerBD);
        auto xrSession = openxr_wrapper_->GetXrSession();
        return xrCreateBodyTrackerBD(xrSession,
                                     createInfo,
                                     bodyTracker);
    }

    int PICOBodyTracking::LocateBodyJoints(XrBodyTrackerBD bodyTracker,
            const XrBodyJointsLocateInfoBD* locateInfo, XrBodyJointLocationsBD* locations) {
        CHECK_POINTER_ARG_IS_NOT_NULL(xrLocateBodyJointsBD);
        return xrLocateBodyJointsBD(bodyTracker,
                                    locateInfo,
                                    locations);
    }

    int PICOBodyTracking::DestroyBodyTracker(XrBodyTrackerBD bodyTracker) {
        CHECK_POINTER_ARG_IS_NOT_NULL(xrDestroyBodyTrackerBD);
        return xrDestroyBodyTrackerBD(bodyTracker);
    }*/

    int PICOBodyTracking::xrSetBodyBoneLength(XrBodyBoneLengthPICO bl) {
        XrBodyBoneLengthPICO bonelength{XR_TYPE_BODY_BONE_LENGTH_PICO};
        bonelength.headBoneLength = bl.headBoneLength;
        bonelength.neckBoneLength = bl.neckBoneLength;
        bonelength.torsoBoneLength = bl.torsoBoneLength;
        bonelength.hipBoneLength = bl.hipBoneLength;
        bonelength.upperBoneLength = bl.upperBoneLength;
        bonelength.lowerBoneLength = bl.lowerBoneLength;
        bonelength.footBoneLength = bl.footBoneLength;
        bonelength.shoulderBoneLength = bl.shoulderBoneLength;
        bonelength.upperArmBoneLength = bl.upperArmBoneLength;
        bonelength.lowerArmBoneLength = bl.lowerArmBoneLength;
        bonelength.handBoneLength = bl.handBoneLength;

        XrBodyTrackerCreateInfoBD createInfo{XR_TYPE_BODY_TRACKER_CREATE_INFO_BD};
        createInfo.jointSet = XR_BODY_JOINT_SET_FULL_BODY_JOINTS_BD;
        createInfo.next = &bonelength;
        CHECK_POINTER_ARG_IS_NOT_NULL(xrCreateBodyTrackerBD);
        auto xrSession = openxr_wrapper_->GetXrSession();
        return xrCreateBodyTrackerBD(xrSession, &createInfo, &body_tracker_);
    }

    int PICOBodyTracking::xrGetBodyTrackingPostureFlag(XrBodyTrackingPostureFlagsDataPICO *postureflags) {
        XrBodyTrackingPostureFlagsDataPICO flags{XR_TYPE_BODY_TRACKING_POSTURE_FLAGS_DATA_PICO};
        flags.jointCount = XR_BODY_JOINT_COUNT_BD;
        flags.postureFlag = flags_;

        XrBodyJointLocationsBD locations{XR_TYPE_BODY_JOINT_LOCATIONS_BD};
        locations.jointLocationCount = XR_BODY_JOINT_COUNT_BD;
        locations.jointLocations = body_joint_location_;
        locations.next = &flags;

        XrBodyJointsLocateInfoBD locateInfo{XR_TYPE_BODY_JOINTS_LOCATE_INFO_BD};
        auto currentFrameIn = openxr_wrapper_->GetCurrentFrameIn();
        locateInfo.time = currentFrameIn.predicted_display_time;
        locateInfo.baseSpace = openxr_wrapper_->GetAppSpace();

        int ret = xrLocateBodyJointsBD(body_tracker_, &locateInfo, &locations);
        if (ret == 0) {
            for (int i = 0; i < XR_BODY_JOINT_COUNT_BD; i++) {
                postureflags->postureFlag[i] = flags_[i];
            }
        }
        return ret;
    }

    int PICOBodyTracking::xrStartBodyTrackingCalibrationApp() {
        CHECK_POINTER_ARG_IS_NOT_NULL(xrStartBodyTrackingCalibrationAppPICO);
        auto xrSession = openxr_wrapper_->GetXrSession();
        return xrStartBodyTrackingCalibrationAppPICO(xrSession);
    }

    int PICOBodyTracking::xrGetBodyTrackingState(XrBodyTrackingStatePICO *state) {
        CHECK_POINTER_ARG_IS_NOT_NULL(xrGetBodyTrackingStatePICO);
        auto xrSession = openxr_wrapper_->GetXrSession();
        return xrGetBodyTrackingStatePICO(xrSession, state);
    }

}  // namespace PVRSampleFW
