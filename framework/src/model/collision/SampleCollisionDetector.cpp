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

#include "SampleCollisionDetector.h"
#include "LogUtils.h"
#include "Intersection.h"
#include "Ray.h"

namespace PVRSampleFW {
    void SampleCollisionDetector::DetectIntersection(std::vector<PVRSampleFW::Scene> *scenes, XrPosef handPose,
                                                     float *outDistance, bool bTrigger, int side) {
        if (side < 0 || side >= 2) {
            PLOGE("SampleCollisionDetector::DetectIntersection invalid side: %d", side);
            return;
        }
        float minDistance = std::numeric_limits<float>::max();
        XrVector3f collidePoint;
        bool bIntersect = false;
        std::shared_ptr<Object> collidedObject = nullptr;

        for (Scene &scene : *scenes) {
            // check each object in the scene
            for (const auto &object : scene.GetAllObjects()) {
                if (!object->IsSolid()) {
                    // skip non-solid object
                    //                    object->RayCollisionResult({0, 0, 0}, false, bTrigger, side);
                    continue;
                }

                if (object->GetType() == ObjectType::OBJECT_TYPE_GUI_PLANE) {
                    glm::vec3 handlePosition(handPose.position.x, handPose.position.y, handPose.position.z);
                    glm::quat handleOrientation(handPose.orientation.w, handPose.orientation.x, handPose.orientation.y,
                                                handPose.orientation.z);
                    auto pose = object->GetPose();
                    glm::vec3 planeCenter(pose.position.x, pose.position.y, pose.position.z);
                    glm::quat planeOrientation(pose.orientation.w, pose.orientation.x, pose.orientation.y,
                                               pose.orientation.z);
                    auto scale = object->GetScale();
                    float planeWidth = scale.x;
                    float planeHeight = scale.y;
                    glm::vec3 pointResult(0);
                    bool bCollision =
                            DetectRayPlaneIntersection(handlePosition, handleOrientation, planeCenter, planeOrientation,
                                                       planeWidth, planeHeight, &pointResult);
                    XrVector3f point{pointResult.x, pointResult.y, pointResult.z};
                    // calculate distance
                    if (bCollision) {
                        auto distance = glm::distance(handlePosition, pointResult);
                        if (distance < minDistance) {
                            minDistance = distance;
                            collidePoint = point;
                            collidedObject = object;
                            bIntersect = true;
                        }
                    }
                } else {
                    auto meshData = object->GetMeshData();
                    if (meshData == nullptr) {
                        // PLOGD("SampleCollisionDetector::DetectIntersection skip detect, mesh data is null");
                        continue;
                    }
                    float distance = FLT_MAX;
                    XrVector3f collideResult{0, 0, 0};
                    XrPosef meshPose = object->GetPose();
                    XrVector3f meshScale = object->GetScale();
                    bool bCollision =
                            DetectRayTriPrimitiveMeshIntersection(handPose.position, handPose.orientation, *meshData,
                                                                  meshScale, meshPose, &collideResult, &distance);

                    if (bCollision && distance < minDistance) {
                        minDistance = distance;
                        collidePoint = collideResult;
                        collidedObject = object;
                        bIntersect = true;
                    }
                }
            }
        }

        if (bIntersect && collidedObject != nullptr) {
            *outDistance = minDistance;
            collidedObject->RayCollisionResult(handPose, collidePoint, bIntersect, bTrigger, side);
        }

        /// inject false collision result to last object if last object is not collide object this frame
        if (last_collided_object_[side] != nullptr && last_collided_object_[side] != collidedObject) {
            last_collided_object_[side]->RayCollisionResult(handPose, {0, 0, 0}, false, bTrigger, side);
        }

        // record this frame's state
        last_collided_object_[side] = collidedObject;
    }

    bool SampleCollisionDetector::DetectRayPlaneIntersection(const glm::vec3 &handlePosition,
                                                             const glm::quat &handleOrientation,
                                                             const glm::vec3 &planeCenter,
                                                             const glm::quat &planeOrientation, float planeWidth,
                                                             float planeHeight, glm::vec3 *outPoint) const {
        glm::vec3 handleDirection = handleOrientation * glm::vec3(0.0f, 0.0f, -1.0f);

        // Calculate the width and height vectors of the plane
        glm::vec3 planeWidthVector = planeWidth * (planeOrientation * glm::vec3(1.0f, 0.0f, 0.0f));
        glm::vec3 planeHeightVector = planeHeight * (planeOrientation * glm::vec3(0.0f, 1.0f, 0.0f));

        // Calculate the normal vector of the plane
        glm::vec3 planeNormal = glm::normalize(glm::cross(planeWidthVector, planeHeightVector));

        // Calculate the distance d of the plane
        float d = -glm::dot(planeNormal, planeCenter);

        // Compute the intersection of a ray and a plane
        float t = -(glm::dot(planeNormal, handlePosition) + d) / glm::dot(planeNormal, handleDirection);
        if (t < 0.0f) {
            // The ray does not intersect the plane
            return false;
        }

        glm::vec3 intersectionPoint = handlePosition + t * handleDirection;

        // Check if the intersection point is in the rectangular plane
        glm::vec3 toPoint = intersectionPoint - planeCenter;
        float projWidth = glm::dot(toPoint, planeWidthVector) / glm::length(planeWidthVector);
        float projHeight = glm::dot(toPoint, planeHeightVector) / glm::length(planeHeightVector);

        float halfWidth = planeWidth / 2.0f;
        float halfHeight = planeHeight / 2.0f;

        if (projWidth >= -halfWidth && projWidth <= halfWidth && projHeight >= -halfHeight &&
            projHeight <= halfHeight) {
            *outPoint = intersectionPoint;
            return true;
        } else {
            // The intersection point is not in the rectangular plane
            return false;
        }

        return false;
    }

    bool SampleCollisionDetector::DetectRayAABBIntersection(const XrVector3f &rayOrigin, const XrQuaternionf &rayQuat,
                                                            const XrVector3f &aabbScale, const XrPosef &aabbPose,
                                                            XrVector3f *outCollidePos, float *distance) {
        PVRSampleFW::Geometry::AABB aabb;
        // set center and extent
        aabb.SetCenterAndExtent({0.0f, 0.0f, 0.0f}, {0.5f * aabbScale.x, 0.5f * aabbScale.y, 0.5f * aabbScale.z});
        // transform aabb
        PVRSampleFW::Geometry::TransformAABB(aabb, aabbPose.position, aabbPose.orientation, &aabb);

        PVRSampleFW::Geometry::Ray ray;
        ray.SetOrigin(rayOrigin);
        XrVector3f rayDir = {0.0f, 0.0f, -1.0f};
        XrQuaternionf_RotateVector3f(&rayDir, &rayQuat, &rayDir);
        ray.SetDirection(rayDir);

        // check ray and aabb
        float t0 = 0.0f;
        float t1 = 0.0f;
        auto bIntersect = PVRSampleFW::Geometry::IntersectRayAABB(ray, aabb, &t0, &t1);
        if (bIntersect) {
            if (t0 > 0.0f) {
                *outCollidePos = ray.GetPoint(t0);
                *distance = t0;
                return true;
            } else {
                /// TODO: from internal to intersect is invalid
                //                outCollidePos = ray.GetPoint(t1);
                //                distance = t1;
                return false;
            }
        }
        return false;
    }

    bool SampleCollisionDetector::DetectRayPlaneIntersection(const XrVector3f &rayOrigin, const XrQuaternionf &rayQuat,
                                                             const XrVector2f &planeScale, const XrPosef &planePose,
                                                             XrVector3f *outCollidePos, float *distance) {
        XrVector3f rayDir = {0.0f, 0.0f, -1.0f};
        XrQuaternionf_RotateVector3f(&rayDir, &rayQuat, &rayDir);
        XrVector3f planeDir = {0.0f, 0.0f, -1.0f};
        XrQuaternionf_RotateVector3f(&planeDir, &planePose.orientation, &planeDir);
        PVRSampleFW::Geometry::Ray ray;
        ray.SetOrigin(rayOrigin);
        ray.SetDirection(rayDir);
        PVRSampleFW::Geometry::Plane plane;
        plane.SetNormalAndPosition(planeDir, planePose.position);
        float t = 0.0f;
        auto bIntersect = PVRSampleFW::Geometry::IntersectRayPlane(ray, plane, &t);
        if (bIntersect) {
            if (t > 0.0f) {
                // check whether the point is in the rectangle
                XrVector2f extent = {0.5f * planeScale.x, 0.5f * planeScale.y};
                XrVector3f point = ray.GetPoint(t);
                XrVector3f toOrigin = MathUtils::subtract(point, planePose.position);
                if (std::abs(toOrigin.x) <= extent.x && std::abs(toOrigin.y) <= extent.y) {
                    *outCollidePos = point;
                    *distance = t;
                    return true;
                }
            } else {
                /// TODO: from behind to intersect is invalid
                return false;
            }
        }

        return false;
    }

    bool SampleCollisionDetector::DetectRayTriPrimitiveMeshIntersection(
            const XrVector3f &rayOrigin, const XrQuaternionf &rayQuat, const Geometry::TriPrimitiveMesh &mesh,
            const XrVector3f &meshScale, const XrPosef &meshPose, XrVector3f *outCollidePos, float *distance) {
        PVRSampleFW::Geometry::Ray ray;
        ray.SetOrigin(rayOrigin);
        XrVector3f rayDir = {0.0f, 0.0f, -1.0f};
        XrQuaternionf_RotateVector3f(&rayDir, &rayQuat, &rayDir);
        ray.SetDirection(rayDir);

        float t = 0.0f;
        auto bIntersect = PVRSampleFW::Geometry::IntersectRayTriPrimitiveMeshWithScaleAndTransform(ray, mesh, meshScale,
                                                                                                   meshPose, &t);
        if (bIntersect) {
            if (t > 0.0f) {
                *outCollidePos = ray.GetPoint(t);
                *distance = t;
                return true;
            } else {
                /// TODO: from behind to intersect is invalid
                return false;
            }
        }
        return false;
    }
}  // namespace PVRSampleFW
