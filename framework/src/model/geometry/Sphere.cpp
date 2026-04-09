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

#include "Sphere.h"

namespace PVRSampleFW {
    namespace Geometry {
        inline void Sphere::Set(const XrVector3f& p0, const XrVector3f& p1) {
            XrVector3f dhalf = MathUtils::multiply(MathUtils::subtract(p1, p0), 0.5f);

            center_ = MathUtils::add(dhalf, p0);
            radius_ = MathUtils::length(dhalf);
        }

        inline bool Sphere::IsInside(const Sphere& inSphere) const {
            if (inSphere.GetRadius() >= GetRadius())
                return false;
            const float squaredDistance =
                    MathUtils::sqrMagnitude(MathUtils::subtract(GetCenter(), inSphere.GetCenter()));
            const float squaredRadius = (GetRadius() - inSphere.GetRadius()) * (GetRadius() - inSphere.GetRadius());
            if (squaredDistance < squaredRadius)
                return true;
            else
                return false;
        }

        inline bool Sphere::IsInside(const XrVector3f& inPoint, float epsilon) const {
            const float squaredDistance = MathUtils::sqrMagnitude(MathUtils::subtract(GetCenter(), inPoint));
            const float squaredRadius = GetRadius() * GetRadius();
            if (squaredDistance <= squaredRadius + epsilon)
                return true;
            else
                return false;
        }

        void Sphere::Set(const XrVector3f* inVertices, uint32_t inHowmany) {
            radius_ = 0.0F;
            center_ = {0.0f, 0.0f, 0.0f};
            uint32_t i;
            for (i = 0; i < inHowmany; i++)
                radius_ = std::max(radius_, MathUtils::sqrMagnitude(inVertices[i]));
            radius_ = sqrt(radius_);
        }

        inline bool Intersect(const Sphere& inSphere0, const Sphere& inSphere1) {
            const float squaredDistance =
                    MathUtils::sqrMagnitude(MathUtils::subtract(inSphere0.GetCenter(), inSphere1.GetCenter()));
            const float squaredRadius =
                    (inSphere0.GetRadius() + inSphere1.GetRadius()) * (inSphere0.GetRadius() + inSphere1.GetRadius());
            if (squaredDistance < squaredRadius)
                return true;
            else
                return false;
        }
    }  // namespace Geometry
}  // namespace PVRSampleFW
