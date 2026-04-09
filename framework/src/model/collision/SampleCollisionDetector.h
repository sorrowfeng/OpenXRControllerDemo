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

#ifndef PICONATIVEOPENXRSAMPLES_SAMPLECOLLISIONDETECTOR_H
#define PICONATIVEOPENXRSAMPLES_SAMPLECOLLISIONDETECTOR_H

#include "ICollisionDetector.h"
#include "glm/ext/quaternion_float.hpp"

namespace PVRSampleFW {

    class SampleCollisionDetector : public ICollisionDetector {
    private:
        SampleCollisionDetector() {
        }

        SampleCollisionDetector(const SampleCollisionDetector&) = delete;
        SampleCollisionDetector& operator=(const SampleCollisionDetector&) = delete;

    public:
        static SampleCollisionDetector* GetInstance() {
            static SampleCollisionDetector instance;
            return &instance;
        }

        void DetectIntersection(std::vector<PVRSampleFW::Scene>* scenes, XrPosef handPose, float* outDistance,
                                bool bTrigger, int side) override;

    private:
        bool DetectRayPlaneIntersection(const glm::vec3& handlePosition, const glm::quat& handleOrientation,
                                        const glm::vec3& planeCenter, const glm::quat& planeOrientation,
                                        float planeWidth, float planeHeight, glm::vec3* outPoint) const;

        bool DetectRayAABBIntersection(const XrVector3f& rayOrigin, const XrQuaternionf& rayQuat,
                                       const XrVector3f& aabbScale, const XrPosef& aabbPose, XrVector3f* outCollidePos,
                                       float* distance);

        bool DetectRayPlaneIntersection(const XrVector3f& rayOrigin, const XrQuaternionf& rayQuat,
                                        const XrVector2f& planeScale, const XrPosef& planePose,
                                        XrVector3f* outCollidePos, float* distance);

        bool DetectRayTriPrimitiveMeshIntersection(const XrVector3f& rayOrigin, const XrQuaternionf& rayQuat,
                                                   const PVRSampleFW::Geometry::TriPrimitiveMesh& mesh,
                                                   const XrVector3f& meshScale, const XrPosef& meshPose,
                                                   XrVector3f* outCollidePos, float* distance);

    private:
        std::shared_ptr<Object> last_collided_object_[2]{nullptr, nullptr};
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_SAMPLECOLLISIONDETECTOR_H
