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

#include "TruncatedCone.h"

namespace PVRSampleFW {
    void TruncatedCone::GenerateMesh(float r1, float r2, float h, int segments) {
        int numVertices = (segments + 1) * 2;
        int numindex = segments * 6;
        vertex_buffer_.resize(numVertices * 6);
        index_buffer_.resize(numindex);
        const float angleStep = M_PI * 2.0f / segments;
        const float kIvoryColor[] = {0.9f, 0.9f, 0.8f, 0.8f};

        struct Vertex {
            float x, y, z;
            float r, g, b, a;
        };
        std::vector<Vertex> vertices;

        // bottom vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = i * angleStep;
            float x = r1 * cos(angle);
            float y = r1 * sin(angle);
            float z = 0.0f;

            Vertex v = {x, y, z, kIvoryColor[0], kIvoryColor[1], kIvoryColor[2], kIvoryColor[3]};
            vertices.push_back(v);
        }
        // top vertices
        for (int i = 0; i <= segments; ++i) {
            float angle = i * angleStep;
            float x = r2 * cos(angle);
            float y = r2 * sin(angle);
            float z = -h;
            Vertex v = {x, y, z, kIvoryColor[0], kIvoryColor[1], kIvoryColor[2], kIvoryColor[3]};
            vertices.push_back(v);
        }
        std::memcpy(vertex_buffer_.data(), vertices.data(), vertices.size() * sizeof(Vertex));

        // index_buffer_ only for lateral
        for (int i = 0; i < segments; ++i) {
            index_buffer_.push_back(i);
            index_buffer_.push_back(i + 1);
            index_buffer_.push_back(segments + 2 + i);

            index_buffer_.push_back(i + 1);
            index_buffer_.push_back(segments + 2 + i);
            index_buffer_.push_back(segments + 2 + (i + 1) % segments);
        }

        // Draw mode
        SetDrawMode(DRAW_MODE_TRIANGLES);

        // Set Draw using array
        SetDrawUsingArrays(true);
    }

    void TruncatedCone::GenerateMesh(float r1, float h, int segments, float slopeAngle) {
        int r2 = r1 - h / tan(slopeAngle);
        GenerateMesh(r1, r2, h, segments);
    }

    void TruncatedCone::GenerateRayMeshAtDefaultRadius(float h, int segments, float slopeAngle) {
        GenerateMesh(1.0f, h, segments, slopeAngle);
    }
}  // namespace PVRSampleFW
