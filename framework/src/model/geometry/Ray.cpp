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

#include "Ray.h"

namespace PVRSampleFW {
    namespace Geometry {
        float Ray::SqrDistToPoint(const XrVector3f &p) const {
            XrVector3f v1 = MathUtils::subtract(p, origin_);

            float c1 = MathUtils::dotProduct(v1, direction_);
            float c2 = MathUtils::dotProduct(direction_, direction_);
            float b = c1 / c2;

            XrVector3f pb = GetPoint(b);

            return MathUtils::length(MathUtils::subtract(pb, p));
        }
    }  // namespace Geometry
}  // namespace PVRSampleFW
