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

//
// Created by ByteDance on 12/18/24.
//

#ifndef PICONATIVEOPENXRSAMPLES_PLANE_H
#define PICONATIVEOPENXRSAMPLES_PLANE_H

#include "xr_linear.h"
#include "MathUtils.h"

namespace PVRSampleFW {
    namespace Geometry {
        // Calculates the unnormalized normal from 3 points
        XrVector3f CalcRawNormalFromTriangle(const XrVector3f& a, const XrVector3f& b, const XrVector3f& c);

        class Plane {
        public:
            XrVector3f normal_;
            float distance_;

            const float& a() const {
                return normal_.x;
            }
            const float& b() const {
                return normal_.y;
            }
            const float& c() const {
                return normal_.z;
            }

            const float& d() const {
                return distance_;
            }
            float& d() {
                return distance_;
            }

            const XrVector3f& GetNormal() const {
                return normal_;
            }

            Plane() {
            }
            Plane(float a, float b, float c, float d) {
                normal_.x = a;
                normal_.y = b;
                normal_.z = c;
                distance_ = d;
            }

            Plane& operator*=(float scale);
            bool operator==(const Plane& p) const {
                return MathUtils::equals(normal_, p.normal_) && distance_ == p.distance_;
            }
            bool operator!=(const Plane& p) const {
                return !MathUtils::equals(normal_, p.normal_) || distance_ != p.distance_;
            }

            void SetInvalid() {
                normal_ = {0.0f, 0.0f, 0.0f};
                distance_ = 0.0F;
            }

            // Just sets the coefficients. Does NOT normalize!
            void SetABCD(const float a, const float b, const float c, const float d);

            void Set3Points(const XrVector3f& a, const XrVector3f& b, const XrVector3f& c);
            bool Set3PointsSafe(const XrVector3f& a, const XrVector3f& b, const XrVector3f& c);
            void Set3PointsUnnormalized(const XrVector3f& a, const XrVector3f& b, const XrVector3f& c);

            void SetNormalAndPosition(const XrVector3f& inNormal, const XrVector3f& inPoint);

            float GetDistanceToPoint(const XrVector3f& input) const;
            float GetDistanceToPoint(const XrVector4f& input) const;
            bool GetSide(const XrVector3f& input) const;
            bool SameSide(const XrVector3f& input0, const XrVector3f& input1);

            //        void NormalizeRobust(float eps = 0.00001f);
            void NormalizeUnsafe();
        };
    }  // namespace Geometry
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_PLANE_H
