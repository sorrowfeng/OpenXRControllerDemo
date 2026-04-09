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

#ifndef PICONATIVEOPENXRSAMPLES_AABB_H
#define PICONATIVEOPENXRSAMPLES_AABB_H

#include "xr_linear.h"
#include <algorithm>
#include "MathUtils.h"

namespace PVRSampleFW {
    namespace Geometry {

        class MinMaxAABB;
        class AABB {
        public:
            XrVector3f center_;
            XrVector3f extent_;

            AABB() : center_({0.0f, 0.0f, 0.0f}), extent_({0.0f, 0.0f, 0.0f}) {
            }

            AABB(const XrVector3f& center, const XrVector3f& extent) {
                center_ = center;
                extent_ = extent;
            }

            explicit AABB(const MinMaxAABB& aabb) {
                FromMinMaxAABB(aabb);
            }

            bool operator==(const AABB& b) const;

            void SetCenterAndExtent(const XrVector3f& center, const XrVector3f& extent) {
                center_ = center;
                extent_ = extent;
            }

            XrVector3f CalculateMin() const {
                return {center_.x - extent_.x, center_.y - extent_.y, center_.z - extent_.z};
            }

            XrVector3f CalculateMax() const {
                return {center_.x + extent_.x, center_.y + extent_.y, center_.z + extent_.z};
            }

            void Expand(float inValue);
            void Scale(const XrVector3f& scale);
            void Offset(const XrVector3f& offset);

            // Encapsulate using AABB is bad for performance and precision
            // Use MinMaxAABB for growing volumes by e.g. encapsulating a set of points
            void SlowLossyEncapsulate(const XrVector3f& inPoint);

            bool IsInside(const XrVector3f& inPoint) const;
            void CalculateVertices(XrVector3f outVertices[8]) const;
            bool IsFinite() const;
            bool IsValid() const;

            // TODO: Get rid of these getters - members are public
            XrVector3f& GetCenter() {
                return center_;
            }
            XrVector3f& GetExtent() {
                return extent_;
            }
            const XrVector3f& GetCenter() const {
                return center_;
            }
            const XrVector3f& GetExtent() const {
                return extent_;
            }

            // TODO: Get rid of 'zero' - it's ambiguous at best.
            // Zero bounds are used to specify 'invalid' bounds in a few places.
            // But in fact it's valid under many operations - and will likely be contained by e.g. another AABB
            static const AABB zero;

        private:
            void FromMinMaxAABB(const MinMaxAABB& aabb);
        };

        bool IsContainedInAABB(const AABB& inside, const AABB& bigBounds);

        // Transforms AABB.
        // Can be thought of as Converting OBB to an AABB:
        // rotate the center and extents of the OBB And find the smallest enclosing AABB around it.
        void TransformAABB(const AABB& aabb, const XrVector3f& position, const XrQuaternionf& rotation, AABB* result);

        /// This is not mathematically correct for non-uniform scaled objects. But it seems to work well enough.
        /// If you use it with non-uniform scale make sure to verify it extensively.
        void TransformAABB(const AABB& aabb, const XrMatrix4x4f& transform, AABB* result);

        /// This version is much slower but works correctly with non-uniform scale
        void TransformAABBSlow(const AABB& aabb, const XrMatrix4x4f& transform, AABB* result);

        /// This version is much slower but works correctly with non-uniform scale
        void TransformAABBSlow(const MinMaxAABB& aabb, const XrMatrix4x4f& transform, MinMaxAABB* result);

        void InverseTransformAABB(const AABB& aabb, const XrVector3f& position, const XrQuaternionf& rotation,
                                  AABB* result);

        /// The closest distance to the surface or inside the aabb.
        float CalculateSqrDistance(const XrVector3f& rkPoint, const AABB& rkBox);

        /// Returns the sqr distance and the closest point inside or on the surface of the aabb.
        /// If inside the aabb, distance will be zero and rkPoint will be returned.
        void CalculateClosestPoint(const XrVector3f& rkPoint, const AABB& rkBox, XrVector3f* outPoint,
                                   float* outSqrDistance);

        /// TODO:Use SIMD library
        /// Compute an AABB using an optional transform
        /// NOTE: When there are many vertices, the caller is responsible for the computation time.
        AABB CalculateAABBFromPositionArray(const XrMatrix4x4f& transform, const XrVector3f* positionArray,
                                            uint32_t positionCount, bool isTransformIdentity = false);

        class MinMaxAABB {
        public:
            XrVector3f min_;
            XrVector3f max_;

            MinMaxAABB() {
                Init();
            }
            MinMaxAABB(const XrVector3f& min, const XrVector3f& max) : min_(min), max_(max) {
            }
            explicit MinMaxAABB(const AABB& aabb) {
                FromAABB(aabb);
            }

            void Init();

            XrVector3f CalculateCenter() const;
            XrVector3f CalculateExtent() const;
            XrVector3f CalculateSize() const;

            void Expand(float inValue);
            void Expand(const XrVector3f& inOffset);

            void Encapsulate(const XrVector3f& inPoint);
            void Encapsulate(const AABB& aabb);
            void Encapsulate(const MinMaxAABB& other);

            bool IsInside(const XrVector3f& inPoint) const;
            void CalculateVertices(XrVector3f outVertices[8]) const;
            bool IsValid() const;

            // TODO: Get rid of these getters - members are public
            const XrVector3f& GetMin() const {
                return min_;
            }
            const XrVector3f& GetMax() const {
                return max_;
            }

        private:
            void FromAABB(const AABB& inAABB);
        };

        inline void AABB::Expand(float inValue) {
            extent_.x += inValue;
            extent_.y += inValue;
            extent_.z += inValue;
        }

        inline void AABB::Scale(const XrVector3f& scale) {
            extent_.x *= scale.x;
            extent_.y *= scale.y;
            extent_.z *= scale.z;
        }

        inline void AABB::Offset(const XrVector3f& offset) {
            center_.x += offset.x;
            center_.y += offset.y;
            center_.z += offset.z;
        }

        inline void AABB::SlowLossyEncapsulate(const XrVector3f& inPoint) {
            XrVector3f bMin, bMax;
            bMin.x = std::min(center_.x - extent_.x, inPoint.x);
            bMin.y = std::min(center_.y - extent_.y, inPoint.y);
            bMin.z = std::min(center_.z - extent_.z, inPoint.z);
            bMax.x = std::max(center_.x + extent_.x, inPoint.x);
            bMax.y = std::max(center_.y + extent_.y, inPoint.y);
            bMax.z = std::max(center_.z + extent_.z, inPoint.z);
            center_.x = (bMin.x + bMax.x) * 0.5f;
            center_.y = (bMin.y + bMax.y) * 0.5f;
            center_.z = (bMin.z + bMax.z) * 0.5f;
            extent_.x = (bMax.x - bMin.x) * 0.5f;
            extent_.y = (bMax.y - bMin.y) * 0.5f;
            extent_.z = (bMax.z - bMin.z) * 0.5f;
        }

        inline bool AABB::IsInside(const XrVector3f& inPoint) const {
            if (std::abs(inPoint.x - center_.x) > extent_.x)
                return false;
            if (std::abs(inPoint.y - center_.y) > extent_.y)
                return false;
            if (std::abs(inPoint.z - center_.z) > extent_.z)
                return false;
            return true;
        }

        inline void AABB::FromMinMaxAABB(const MinMaxAABB& aabb) {
            center_ = {(aabb.max_.x + aabb.min_.x) * 0.5f, (aabb.max_.y + aabb.min_.y) * 0.5f,
                       (aabb.max_.z + aabb.min_.z) * 0.5f};
            extent_ = {(aabb.max_.x - aabb.min_.x) * 0.5f, (aabb.max_.y - aabb.min_.y) * 0.5f,
                       (aabb.max_.z - aabb.min_.z) * 0.5f};
        }

    }  // namespace Geometry
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_AABB_H
