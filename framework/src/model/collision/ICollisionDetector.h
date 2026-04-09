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

#ifndef PICONATIVEOPENXRSAMPLES_ICOLLISIONDETECTOR_H
#define PICONATIVEOPENXRSAMPLES_ICOLLISIONDETECTOR_H

#include "Scene.h"

namespace PVRSampleFW {

    class ICollisionDetector {
    public:
        virtual ~ICollisionDetector() = default;

        /**
         * Perform collision detection
         * @param scenes All objects to be collided
         * @param handPose Describing the Ray Position
         * @param bTrigger Key information, whether to trigger
         * @param side 0 is left, 1 is right
         */
        virtual void DetectIntersection(std::vector<PVRSampleFW::Scene>* scenes, XrPosef handPose, float* outDistance,
                                        bool bTrigger, int side) = 0;
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_ICOLLISIONDETECTOR_H
