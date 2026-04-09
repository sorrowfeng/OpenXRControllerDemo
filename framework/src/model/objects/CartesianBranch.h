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

#ifndef PICONATIVEOPENXRSAMPLES_CARTESIANBRANCH_H
#define PICONATIVEOPENXRSAMPLES_CARTESIANBRANCH_H

#include "Object.h"

namespace PVRSampleFW {
    static constexpr float CARTESIAN_BRANCH_VERTICES[] = {
            // x - red
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            // y - green
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            // z - blue
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            0.0f,
            -1.0f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
    };

    static constexpr uint32_t CARTESIAN_BRANCH_INDICES[] = {
            // x
            0,
            1,
            // y
            2,
            3,
            // z
            4,
            5,
    };

    /**
     * A object that describe a shape named "cartesian coordinate system" (3 dimension)
     *
     * Like this:
     *        Y
     *        |
     *        |
     *        |   -Z
     *        |  /
     *        | /
     *        |/
     *   -----+------------ X
     *      0 |
     *        |
     */
    class CartesianBranch : public Object {
    public:
        CartesianBranch() : Object() {
            type_ = OBJECT_TYPE_CARTESIAN_BRANCH;
            gl_program_type_ = GL_PROGRAM_TYPE_COLOR;
            gl_geometry_type_ = GL_GEOMETRY_TYPE_POS_COLOR_CARTESIAN_BRANCH;
            // Draw mode
            SetDrawMode(DRAW_MODE_LINES);
            // Set Draw using array
            SetDrawUsingArrays(false);

            Build();
        }

        CartesianBranch(const XrPosef& p, const XrVector3f& s) : Object() {
            type_ = OBJECT_TYPE_CARTESIAN_BRANCH;
            gl_program_type_ = GL_PROGRAM_TYPE_COLOR;
            gl_geometry_type_ = GL_GEOMETRY_TYPE_POS_COLOR_CARTESIAN_BRANCH;
            this->pose_ = p;
            this->scale_ = s;
            // Draw mode
            SetDrawMode(DRAW_MODE_LINES);
            // Set Draw using array
            SetDrawUsingArrays(false);

            Build();
        }

        virtual ~CartesianBranch() = default;

    private:
        void Build();
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_CARTESIANBRANCH_H
