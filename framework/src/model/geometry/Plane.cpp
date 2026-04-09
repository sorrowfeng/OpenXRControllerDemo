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

#include "Plane.h"
namespace PVRSampleFW {
    namespace Geometry {
        inline XrVector3f CalcRawNormalFromTriangle(const XrVector3f &a, const XrVector3f &b, const XrVector3f &c) {
            return MathUtils::crossProduct(MathUtils::subtract(a, b), MathUtils::subtract(a, c));
        }

        inline Plane &Plane::operator*=(float scale) {
            normal_ = MathUtils::multiply(normal_, scale);
            distance_ *= scale;
            return *this;
        }

        inline void Plane::SetABCD(float a, float b, float c, float d) {
            normal_ = {a, b, c};
            distance_ = d;
        }

        void Plane::Set3Points(const XrVector3f &a, const XrVector3f &b, const XrVector3f &c) {
            normal_ = CalcRawNormalFromTriangle(a, b, c);
            normal_ = MathUtils::normalize(normal_);
            distance_ = -MathUtils::dotProduct(normal_, a);
        }

        bool Plane::Set3PointsSafe(const XrVector3f &a, const XrVector3f &b, const XrVector3f &c) {
            normal_ = CalcRawNormalFromTriangle(a, b, c);

            float length = MathUtils::length(normal_);
            if (length < 0.00001F) {
                return false;
            }

            normal_ = MathUtils::divide(normal_, length);
            distance_ = -MathUtils::dotProduct(normal_, a);
            return true;
        }

        void Plane::Set3PointsUnnormalized(const XrVector3f &a, const XrVector3f &b, const XrVector3f &c) {
            normal_ = CalcRawNormalFromTriangle(a, b, c);
            distance_ = -MathUtils::dotProduct(normal_, a);
        }

        void Plane::SetNormalAndPosition(const XrVector3f &inNormal, const XrVector3f &inPoint) {
            normal_ = inNormal;
            distance_ = -MathUtils::dotProduct(inNormal, inPoint);
        }

        float Plane::GetDistanceToPoint(const XrVector3f &input) const {
            return MathUtils::dotProduct(normal_, input) + distance_;
        }

        // input w component is ignored from distance computations
        float Plane::GetDistanceToPoint(const XrVector4f &input) const {
            return normal_.x * input.x + normal_.y * input.y + normal_.z * input.z + distance_;
        }

        // Returns true if we are on the front side (same as: GetDistanceToPoint () > 0.0)
        bool Plane::GetSide(const XrVector3f &input) const {
            return (MathUtils::dotProduct(normal_, input) + distance_) > 0.0f;
        }

        bool Plane::SameSide(const XrVector3f &inPt0, const XrVector3f &inPt1) {
            float d0 = GetDistanceToPoint(inPt0);
            float d1 = GetDistanceToPoint(inPt1);
            if (d0 > 0.0f && d1 > 0.0f)
                return true;
            else if (d0 <= 0.0f && d1 <= 0.0f)
                return true;
            else
                return false;
        }

        void Plane::NormalizeUnsafe() {
            float invMag = 1.0f / MathUtils::length(normal_);
            normal_ = MathUtils::multiply(normal_, invMag);
            distance_ *= invMag;
        }
    }  // namespace Geometry
}  // namespace PVRSampleFW
