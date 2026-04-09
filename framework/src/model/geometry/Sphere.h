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
// Created by ByteDance on 12/19/24.
//

#ifndef PICONATIVEOPENXRSAMPLES_SPHERE_H
#define PICONATIVEOPENXRSAMPLES_SPHERE_H

#include "xr_linear.h"
#include "MathUtils.h"
#include <algorithm>

namespace PVRSampleFW {
    namespace Geometry {

        class Sphere {
            XrVector3f center_;
            float radius_;

        public:
            Sphere() {
            }
            Sphere(const XrVector3f& p0, float r) {
                Set(p0, r);
            }

            void Set(const XrVector3f& p0) {
                center_ = p0;
                radius_ = 0;
            }
            void Set(const XrVector3f& p0, float r) {
                center_ = p0;
                radius_ = r;
            }

            void Set(const XrVector3f& p0, const XrVector3f& p1);

            void Set(const XrVector3f* inVertices, uint32_t inHowmany);

            XrVector3f& GetCenter() {
                return center_;
            }
            const XrVector3f& GetCenter() const {
                return center_;
            }

            float& GetRadius() {
                return radius_;
            }
            const float& GetRadius() const {
                return radius_;
            }

            bool IsInside(const Sphere& inSphere) const;
            bool IsInside(const XrVector3f& inPoint, float epsilon = 0.f) const;
        };

        bool Intersect(const Sphere& inSphere0, const Sphere& inSphere1);

    }  // namespace Geometry
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_SPHERE_H
