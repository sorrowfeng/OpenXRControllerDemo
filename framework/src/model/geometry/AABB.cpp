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

#include "AABB.h"

namespace PVRSampleFW {
    namespace Geometry {
        const AABB AABB::zero = AABB({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
        bool AABB::operator==(const AABB& b) const {
            return center_.x == b.center_.x && center_.y == b.center_.y && center_.z == b.center_.z &&
                   extent_.x == b.extent_.x && extent_.y == b.extent_.y && extent_.z == b.extent_.z;
        }

        void AABB::CalculateVertices(XrVector3f outVertices[8]) const {
            outVertices[0] = {center_.x - extent_.x, center_.y - extent_.y, center_.z - extent_.z};
            outVertices[1] = {center_.x + extent_.x, center_.y - extent_.y, center_.z - extent_.z};
            outVertices[2] = {center_.x - extent_.x, center_.y + extent_.y, center_.z - extent_.z};
            outVertices[3] = {center_.x + extent_.x, center_.y + extent_.y, center_.z - extent_.z};

            outVertices[4] = {center_.x - extent_.x, center_.y - extent_.y, center_.z + extent_.z};
            outVertices[5] = {center_.x + extent_.x, center_.y - extent_.y, center_.z + extent_.z};
            outVertices[6] = {center_.x - extent_.x, center_.y + extent_.y, center_.z + extent_.z};
            outVertices[7] = {center_.x + extent_.x, center_.y + extent_.y, center_.z + extent_.z};
        }

        bool AABB::IsFinite() const {
            return std::isfinite(center_.x) && std::isfinite(center_.y) && std::isfinite(center_.z) &&
                   std::isfinite(extent_.x) && std::isfinite(extent_.y) && std::isfinite(extent_.z);
        }

        bool AABB::IsValid() const {
            return !IsFinite() && !(*this == AABB::zero);
        }

        bool IsContainedInAABB(const AABB& inside, const AABB& bigBounds) {
            bool outside = false;
            outside |= (inside.center_.x - inside.extent_.x) < (bigBounds.center_.x - bigBounds.extent_.x);
            outside |= (inside.center_.x + inside.extent_.x) > (bigBounds.center_.x + bigBounds.extent_.x);

            outside |= (inside.center_.y - inside.extent_.y) < (bigBounds.center_.y - bigBounds.extent_.y);
            outside |= (inside.center_.y + inside.extent_.y) > (bigBounds.center_.y + bigBounds.extent_.y);

            outside |= (inside.center_.z - inside.extent_.z) < (bigBounds.center_.z - bigBounds.extent_.z);
            outside |= (inside.center_.z + inside.extent_.z) > (bigBounds.center_.z + bigBounds.extent_.z);

            return !outside;
        }

        XrVector3f RotateExtents(const XrVector3f& extents, const XrMatrix4x4f& rotation) {
            XrVector3f newExtents;
            newExtents.x = std::abs(rotation.m[0] * extents.x) + std::abs(rotation.m[4] * extents.y) +
                           std::abs(rotation.m[8] * extents.z);
            newExtents.y = std::abs(rotation.m[1] * extents.x) + std::abs(rotation.m[5] * extents.y) +
                           std::abs(rotation.m[9] * extents.z);
            newExtents.z = std::abs(rotation.m[2] * extents.x) + std::abs(rotation.m[6] * extents.y) +
                           std::abs(rotation.m[10] * extents.z);
            return newExtents;
        }

        void TransformAABB(const AABB& aabb, const XrVector3f& position, const XrQuaternionf& rotation, AABB* result) {
            XrMatrix4x4f rotationMatrix;
            XrMatrix4x4f_CreateFromQuaternion(&rotationMatrix, &rotation);

            XrVector3f transformedCenter;
            XrMatrix4x4f_TransformVector3f(&transformedCenter, &rotationMatrix, &aabb.center_);

            XrVector3f newCenter = MathUtils::add(position, transformedCenter);
            XrVector3f newExtents = {
                    fabs(aabb.extent_.x * rotationMatrix.m[0]) + fabs(aabb.extent_.y * rotationMatrix.m[4]) +
                            fabs(aabb.extent_.z * rotationMatrix.m[8]),
                    fabs(aabb.extent_.x * rotationMatrix.m[1]) + fabs(aabb.extent_.y * rotationMatrix.m[5]) +
                            fabs(aabb.extent_.z * rotationMatrix.m[9]),
                    fabs(aabb.extent_.x * rotationMatrix.m[2]) + fabs(aabb.extent_.y * rotationMatrix.m[6]) +
                            fabs(aabb.extent_.z * rotationMatrix.m[10])};
            result->center_ = newCenter;
            result->extent_ = newExtents;
        }

        void TransformAABB(const AABB& aabb, const XrMatrix4x4f& transform, AABB* result) {
            XrVector3f transformedCenter;
            XrMatrix4x4f_TransformVector3f(&transformedCenter, &transform, &aabb.center_);
            XrVector3f newCenter = MathUtils::add(transformedCenter, aabb.center_);
            XrVector3f newExtents = RotateExtents(aabb.extent_, transform);
            result->center_ = newCenter;
            result->extent_ = newExtents;
        }

        void TransformAABBSlow(const AABB& aabb, const XrMatrix4x4f& transform, AABB* result) {
            XrVector3f vertices[8];
            aabb.CalculateVertices(vertices);
            MinMaxAABB transformed;
            for (int i = 0; i < 8; ++i) {
                XrVector3f transformedVertex;
                XrMatrix4x4f_TransformVector3f(&transformedVertex, &transform, &vertices[i]);
                transformed.Encapsulate(transformedVertex);
            }

            *result = AABB(transformed);
        }

        void TransformAABBSlow(const MinMaxAABB& aabb, const XrMatrix4x4f& transform, MinMaxAABB* result) {
            XrVector3f v[8];
            aabb.CalculateVertices(v);

            result->Init();
            for (int i = 0; i < 8; i++) {
                XrVector3f point;
                XrMatrix4x4f_TransformVector3f(&point, &transform, &v[i]);
                result->Encapsulate(point);
            }
        }

        void InverseTransformAABB(const AABB& aabb, const XrVector3f& position, const XrQuaternionf& rotation,
                                  AABB* result) {
            XrQuaternionf inverseRotation;
            XrQuaternionf_Invert(&inverseRotation, &rotation);
            XrMatrix4x4f m;
            XrMatrix4x4f_CreateFromQuaternion(&m, &inverseRotation);

            XrVector3f extents = RotateExtents(aabb.extent_, m);
            XrVector3f center = MathUtils::subtract(aabb.center_, position);
            XrVector3f centerTransformed;
            XrMatrix4x4f_TransformVector3f(&centerTransformed, &m, &center);

            result->SetCenterAndExtent(centerTransformed, extents);
        }

        void MinMaxAABB::Init() {
            min_ = {FLT_MAX, FLT_MAX, FLT_MAX};
            max_ = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
        }

        XrVector3f MinMaxAABB::CalculateCenter() const {
            return MathUtils::divide(MathUtils::add(min_, max_), 2.0f);
        }

        XrVector3f MinMaxAABB::CalculateExtent() const {
            return MathUtils::divide(MathUtils::subtract(max_, min_), 2.0f);
        }

        XrVector3f MinMaxAABB::CalculateSize() const {
            return MathUtils::subtract(max_, min_);
        }

        void MinMaxAABB::Expand(float inValue) {
            min_ = MathUtils::subtract(min_, {inValue, inValue, inValue});
            max_ = MathUtils::add(max_, {inValue, inValue, inValue});
        }

        void MinMaxAABB::Expand(const XrVector3f& inOffset) {
            min_ = MathUtils::subtract(min_, inOffset);
            max_ = MathUtils::add(max_, inOffset);
        }

        void MinMaxAABB::Encapsulate(const XrVector3f& inPoint) {
            min_ = MathUtils::min(min_, inPoint);
            max_ = MathUtils::max(max_, inPoint);
        }

        void MinMaxAABB::Encapsulate(const AABB& aabb) {
            min_ = MathUtils::min(min_, aabb.CalculateMin());
            max_ = MathUtils::max(max_, aabb.CalculateMax());
        }

        void MinMaxAABB::Encapsulate(const MinMaxAABB& other) {
            min_ = MathUtils::min(min_, other.min_);
            max_ = MathUtils::max(max_, other.max_);
        }

        bool MinMaxAABB::IsInside(const XrVector3f& inPoint) const {
            return inPoint.x >= min_.x && inPoint.x <= max_.x && inPoint.y >= min_.y && inPoint.y <= max_.y &&
                   inPoint.z >= min_.z && inPoint.z <= max_.z;
        }

        void MinMaxAABB::CalculateVertices(XrVector3f* outVertices) const {
            //    7-----6
            //   /     /|
            //  3-----2 |
            //  | 4   | 5
            //  |     |/
            //  0-----1
            outVertices[0] = {min_.x, min_.y, min_.z};
            outVertices[1] = {max_.x, min_.y, min_.z};
            outVertices[2] = {max_.x, max_.y, min_.z};
            outVertices[3] = {min_.x, max_.y, min_.z};
            outVertices[4] = {min_.x, min_.y, max_.z};
            outVertices[5] = {max_.x, min_.y, max_.z};
            outVertices[6] = {max_.x, max_.y, max_.z};
            outVertices[7] = {min_.x, max_.y, max_.z};
        }

        bool MinMaxAABB::IsValid() const {
            XrVector3f infinite = {FLT_MAX, FLT_MAX, FLT_MAX};
            XrVector3f negativeInfinite = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
            return !(MathUtils::equals(min_, infinite) || MathUtils::equals(max_, negativeInfinite));
        }

        void MinMaxAABB::FromAABB(const AABB& inAABB) {
            min_ = inAABB.CalculateMin();
            max_ = inAABB.CalculateMax();
        }

        AABB CalculateAABBFromPositionArray(const XrMatrix4x4f& transform, const XrVector3f* positionArray,
                                            uint32_t positionCount, bool isTransformIdentity) {
            if (!positionArray || !positionCount) {
                return AABB({0.0f, 0.0f, 0.0f}, {FLT_MAX, FLT_MAX, FLT_MAX});
            }

            if (isTransformIdentity) {
                MinMaxAABB aabb;
                aabb.Init();
                for (uint32_t i = 0; i < positionCount; i++) {
                    aabb.Encapsulate(positionArray[i]);
                }
                return AABB(aabb);
            } else {
                MinMaxAABB aabb;
                aabb.Init();
                for (uint32_t i = 0; i < positionCount; i++) {
                    XrVector3f transformedPosition;
                    XrMatrix4x4f_TransformVector3f(&transformedPosition, &transform, &positionArray[i]);
                    aabb.Encapsulate(transformedPosition);
                }
                return AABB(aabb);
            }
        }

    }  // namespace Geometry
}  // namespace PVRSampleFW
