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

#include "BDBodyTracking.h"
#include "BasicOpenXrWrapper.h"

namespace PVRSampleFW {
    std::vector<std::string> BDBodyTracking::GetRequiredExtensions() const {
        return {XR_BD_BODY_TRACKING_EXTENSION_NAME};
    }

    bool BDBodyTracking::OnInstanceCreate() {
        // Initialize the function pointers
        if (openxr_wrapper_ == nullptr) {
            PLOGE("BDBodyTracking::OnInstanceCreate failed for openxr_wrapper_ is null");
            return false;
        }

        auto is_bd_body_tracking_enabled = openxr_wrapper_->IsExtensionEnabled(XR_BD_BODY_TRACKING_EXTENSION_NAME);
        PLOGE("BDBodyTrackingInit::OnInstanceCreate is_bd_body_tracking_enabled %d", is_bd_body_tracking_enabled);

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrCreateBodyTrackerBD",
                                          reinterpret_cast<PFN_xrVoidFunction*>(&xrCreateBodyTrackerBD)));

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrDestroyBodyTrackerBD",
                                          reinterpret_cast<PFN_xrVoidFunction*>(&xrDestroyBodyTrackerBD)));

        CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrLocateBodyJointsBD",
                                          reinterpret_cast<PFN_xrVoidFunction*>(&xrLocateBodyJointsBD)));

        return true;
    }

    bool BDBodyTracking::OnSystemGet(XrSystemProperties* configProperties) {
        configProperties->next = &body_tracking_system_properties_;
        return true;
    }

    int BDBodyTracking::CreateBodyTracker(const XrBodyTrackerCreateInfoBD* createInfo, XrBodyTrackerBD* bodyTracker) {

        PLOGE("BDBodyTracking::CreateBodyTracker");
        CHECK_POINTER_ARG_IS_NOT_NULL(xrCreateBodyTrackerBD);
        auto xrSession = openxr_wrapper_->GetXrSession();
        return xrCreateBodyTrackerBD(xrSession, createInfo, bodyTracker);
    }

    int BDBodyTracking::LocateBodyJoints(XrBodyTrackerBD bodyTracker, const XrBodyJointsLocateInfoBD* locateInfo,
                                         XrBodyJointLocationsBD* locations) {
        CHECK_POINTER_ARG_IS_NOT_NULL(xrLocateBodyJointsBD);
        return xrLocateBodyJointsBD(bodyTracker, locateInfo, locations);
    }

    int BDBodyTracking::DestroyBodyTracker(XrBodyTrackerBD bodyTracker) {
        CHECK_POINTER_ARG_IS_NOT_NULL(xrDestroyBodyTrackerBD);
        return xrDestroyBodyTrackerBD(bodyTracker);
    }

    bool BDBodyTracking::OnSessionCreate() {
        if (NULL == body_tracker_) {
            XrBodyTrackerCreateInfoBD createInfo = {XR_TYPE_BODY_TRACKER_CREATE_INFO_BD};
            createInfo.jointSet = XR_BODY_JOINT_SET_FULL_BODY_JOINTS_BD;
            return CreateBodyTracker(&createInfo, &body_tracker_);
        }
        return true;
    }

    bool BDBodyTracking::OnSessionEnd() {
        if (NULL == body_tracker_) {
            return DestroyBodyTracker(body_tracker_);
        }
        return true;
    }

}  // namespace PVRSampleFW
