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

#ifndef PICONATIVEOPENXRSAMPLES_BDBODYTRACKING_H
#define PICONATIVEOPENXRSAMPLES_BDBODYTRACKING_H
#include "IXrPlatformPlugin.h"
#include "openxr/openxr.h"
#include "IOpenXRExtensionPlugin.h"
#include "CheckUtils.h"

namespace PVRSampleFW {

    class BDBodyTracking : public IOpenXRExtensionPlugin {
    public:
        virtual ~BDBodyTracking() = default;

        BDBodyTracking() = default;
        BDBodyTracking(const std::shared_ptr<IXrPlatformPlugin>& plaf, BasicOpenXrWrapper* openxrW)
            : IOpenXRExtensionPlugin(plaf, openxrW) {
        }

        std::vector<std::string> GetRequiredExtensions() const override;

        bool OnInstanceCreate() override;

        bool OnSystemGet(XrSystemProperties* configProperties) override;

        int CreateBodyTracker(const XrBodyTrackerCreateInfoBD* createInfo, XrBodyTrackerBD* bodyTracker);

        int LocateBodyJoints(XrBodyTrackerBD bodyTracker, const XrBodyJointsLocateInfoBD* locateInfo,
                             XrBodyJointLocationsBD* locations);

        int DestroyBodyTracker(XrBodyTrackerBD bodyTracker);

        bool OnSessionCreate() override;

        bool OnSessionEnd() override;

    public:
        PFN_DECLARE(xrCreateBodyTrackerBD);
        PFN_DECLARE(xrDestroyBodyTrackerBD);
        PFN_DECLARE(xrLocateBodyJointsBD);

        XrSystemBodyTrackingPropertiesBD body_tracking_system_properties_;
        XrBodyTrackerBD body_tracker_ = XR_NULL_HANDLE;
        XrBodyJointLocationBD body_joint_location_[XR_BODY_JOINT_COUNT_BD];
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_BDBODYTRACKING_H
