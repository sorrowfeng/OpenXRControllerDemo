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

#ifndef PICONATIVEOPENXRSAMPLES_RAY_H
#define PICONATIVEOPENXRSAMPLES_RAY_H

#include "xr_linear.h"
#include "MathUtils.h"

namespace PVRSampleFW {
    namespace Geometry {
        class Ray {
            XrVector3f origin_;
            XrVector3f direction_;  // Direction is always normalized

        public:
            Ray() {
            }
            Ray(const XrVector3f& orig, const XrVector3f& dir) {
                origin_ = orig;
                SetDirection(dir);
            }

            const XrVector3f& GetDirection() const {
                return direction_;
            }
            // Direction has to be normalized
            void SetDirection(const XrVector3f& dir) {
                direction_ = MathUtils::normalize(dir);
            }
            void SetOrigin(const XrVector3f& origin) {
                origin_ = origin;
            }

            const XrVector3f& GetOrigin() const {
                return origin_;
            }
            XrVector3f GetPoint(float t) const {
                return MathUtils::add(origin_, MathUtils::multiply(direction_, t));
            }

            float SqrDistToPoint(const XrVector3f& v) const;
        };
    }  // namespace Geometry
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_RAY_H
