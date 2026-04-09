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
// Created by ByteDance on 11/28/24.
//

#include "CartesianBranch.h"

namespace PVRSampleFW {
    void CartesianBranch::Build() {
        // vertex buffer
        auto vertexSize = sizeof(CARTESIAN_BRANCH_VERTICES);
        vertex_buffer_.resize(vertexSize / sizeof(float));
        std::memcpy(vertex_buffer_.data(), CARTESIAN_BRANCH_VERTICES, vertexSize);

        // index buffer
        auto indexSize = sizeof(CARTESIAN_BRANCH_INDICES);
        index_buffer_.resize(indexSize / sizeof(uint32_t));
        std::memcpy(index_buffer_.data(), CARTESIAN_BRANCH_INDICES, indexSize);

        // Draw mode
        SetDrawMode(DRAW_MODE_LINES);

        // Set Draw using array
        SetDrawUsingArrays(false);
    }
}  // namespace PVRSampleFW
