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
// Created by ByteDance on 11/25/24.
//

#ifndef PICONATIVEOPENXRSAMPLES_TRUNCATEDCONE_H
#define PICONATIVEOPENXRSAMPLES_TRUNCATEDCONE_H

#include "Object.h"

namespace PVRSampleFW {

    class TruncatedCone : public Object {
    public:
        TruncatedCone() : Object() {
            type_ = ObjectType::OBJECT_TYPE_TRUNCATED_CONE;
            gl_program_type_ = GL_PROGRAM_TYPE_COLOR;
            gl_geometry_type_ = GL_GEOMETRY_TYPE_POS_COLOR_TRUNCATED_CONE;
            // Draw mode
            SetDrawMode(DRAW_MODE_TRIANGLES);

            // Set Draw using array
            SetDrawUsingArrays(true);
        }

        TruncatedCone(const XrPosef& p, const XrVector3f& s) : Object() {
            type_ = ObjectType::OBJECT_TYPE_TRUNCATED_CONE;
            gl_program_type_ = GL_PROGRAM_TYPE_COLOR;
            gl_geometry_type_ = GL_GEOMETRY_TYPE_POS_COLOR_TRUNCATED_CONE;
            this->pose_ = p;
            this->scale_ = s;
            // Draw mode
            SetDrawMode(DRAW_MODE_TRIANGLES);

            // Set Draw using array
            SetDrawUsingArrays(true);
        }

        TruncatedCone(int segments, float slopeAngle) : TruncatedCone() {
            GenerateRayMeshAtDefaultRadius(1.0f, segments, slopeAngle);
        }

        TruncatedCone(const XrPosef& p, const XrVector3f& s, int segments, float slopeAngle) : TruncatedCone(p, s) {
            GenerateRayMeshAtDefaultRadius(1.0f, segments, slopeAngle);
        }

        /**
         * Generate a truncated cone mesh by radius and height.
         * @param r1 near bottom radius
         * @param r2 far top radius
         * @param h height
         * @param segments segments number
         */
        void GenerateMesh(float r1, float r2, float h, int segments);

        /**
         * Generate a truncated cone mesh by radius, height and slope angle.
         * @param r1 near bottom radius
         * @param h height
         * @param segments segments number
         * @param slopeAngle angle of slope
         */
        void GenerateMesh(float r1, float h, int segments, float slopeAngle);

        /**
         * Generate a ray mesh at default radius.
         * @param h length of ray
         * @param segments segments number
         * @param slopeAngle angle of slope
         */
        void GenerateRayMeshAtDefaultRadius(float h, int segments, float slopeAngle);
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_TRUNCATEDCONE_H
