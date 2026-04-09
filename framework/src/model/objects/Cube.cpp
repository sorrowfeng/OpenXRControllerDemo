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

#include "Cube.h"
#include "LogUtils.h"
#include "AABB.h"

namespace PVRSampleFW {

    void Cube::BuildObjectV2() {
        // vertex buffer
        auto vertexCnt = sizeof(CUBE_POS_VERTICES) / sizeof(XrVector3f);
        auto vertexSize = sizeof(PosColorVertex) * vertexCnt;  // position + color
        vertex_buffer_.resize(vertexSize / sizeof(float));

        // build up mesh vertex attribute
        std::vector<PosColorVertex> vertices(vertexCnt);
        for (uint32_t i = 0; i < vertexCnt; i++) {
            vertices[i].position = CUBE_POS_VERTICES[i];
            vertices[i].color = CUBE_COLOR_DATA[i];
        }
        std::memcpy(vertex_buffer_.data(), vertices.data(), vertexSize);

        // index buffer
        auto indexSize = sizeof(CUBE_INDICES);
        auto indexCnt = indexSize / sizeof(uint32_t);
        index_buffer_.resize(indexSize / sizeof(uint32_t));
        std::memcpy(index_buffer_.data(), CUBE_INDICES, indexSize);

        // Draw mode
        SetDrawMode(DRAW_MODE_TRIANGLES);

        // Set Draw using array
        SetDrawUsingArrays(true);

        // set solid
        SetSolid(true);

        // set movable
        // TODO:debug
        SetMovable(true);
    }

    void Cube::BuildObjectV1() {
        // vertex buffer
        auto vertexCnt = sizeof(CUBE_POS_VERTICES) / sizeof(XrVector3f);
        auto vertexSize = sizeof(PosColorVertex) * vertexCnt;  // position + color
        vertex_buffer_.resize(vertexSize / sizeof(float));

        // build up mesh vertex attribute
        std::vector<PosColorVertex> vertices(vertexCnt);
        for (uint32_t i = 0; i < vertexCnt; i++) {
            vertices[i].position = CUBE_POS_VERTICES[i];
            vertices[i].color = CUBE_COLOR_DATA[i];
        }
        std::memcpy(vertex_buffer_.data(), vertices.data(), vertexSize);

        // Draw mode
        SetDrawMode(DRAW_MODE_TRIANGLES);

        // Set Draw using array
        SetDrawUsingArrays(true);

        // set solid
        SetSolid(true);

        // set movable
        // TODO:debug
        SetMovable(true);
    }

    void Cube::BuildObjectV3() {
        // vertex buffer
        auto vertexSize = sizeof(CUBE_POS_VERTICES);
        vertex_buffer_.resize(vertexSize / sizeof(float));
        std::memcpy(vertex_buffer_.data(), CUBE_POS_VERTICES, vertexSize);

        // color buffer
        auto colorSize = sizeof(CUBE_COLOR_DATA);
        color_buffer_.resize(colorSize / sizeof(float));
        std::memcpy(color_buffer_.data(), CUBE_COLOR_DATA, colorSize);

        // normal buffer
        auto normalSize = sizeof(CUBE_NORMAL_DATA);
        normals_buffer_.resize(normalSize / sizeof(float));
        std::memcpy(normals_buffer_.data(), CUBE_NORMAL_DATA, normalSize);

        // Draw mode
        SetDrawMode(DRAW_MODE_TRIANGLES);

        // Set Draw using array
        SetDrawUsingArrays(true);

        // set solid
        SetSolid(true);

        // set movable
        // TODO:debug
        SetMovable(true);
    }

    void Cube::BuildObjectV4(const std::vector<uint8_t> &textureData) {
        // vertex buffer
        auto vertexSize = sizeof(CUBE_POS_VERTICES);
        vertex_buffer_.resize(vertexSize / sizeof(float));
        std::memcpy(vertex_buffer_.data(), CUBE_POS_VERTICES, vertexSize);

        // texture buffer
        auto textureSize = sizeof(CUBE_TEXTURE_COORDINATE_DATA);
        texture_buffer_.resize(textureSize / sizeof(float));
        std::memcpy(texture_buffer_.data(), CUBE_TEXTURE_COORDINATE_DATA, textureSize);

        // texture date
        ///TODO: set Material texture data

        // Draw mode
        SetDrawMode(DRAW_MODE_TRIANGLES);

        // Set Draw using array
        SetDrawUsingArrays(true);

        // set solid
        SetSolid(true);

        // set movable
        // TODO:debug
        SetMovable(true);
    }

    void Cube::BuildObjectV5(const std::vector<uint8_t> &textureData) {
        // vertex buffer
        auto vertexSize = sizeof(CUBE_POS_VERTICES);
        vertex_buffer_.resize(vertexSize / sizeof(float));
        std::memcpy(vertex_buffer_.data(), CUBE_POS_VERTICES, vertexSize);

        // color buffer
        auto colorSize = sizeof(CUBE_COLOR_DATA_WITH_TRANSPARENCY);
        color_buffer_.resize(colorSize / sizeof(float));
        std::memcpy(color_buffer_.data(), CUBE_COLOR_DATA_WITH_TRANSPARENCY, colorSize);

        // texture buffer
        auto textureSize = sizeof(CUBE_TEXTURE_COORDINATE_DATA);
        texture_buffer_.resize(textureSize / sizeof(float));
        std::memcpy(texture_buffer_.data(), CUBE_TEXTURE_COORDINATE_DATA, textureSize);

        // texture date
        ///TODO: set Material texture data

        // Draw mode
        SetDrawMode(DRAW_MODE_TRIANGLES);

        // Set Draw using array
        SetDrawUsingArrays(true);

        // set solid
        SetSolid(true);

        // set movable
        // TODO:debug
        SetMovable(true);
    }

    void Cube::BuildMeshData() {
        meshData = std::make_shared<PVRSampleFW::Geometry::TriPrimitiveMesh>();
        // set vertices and indices
        auto vertexCnt = sizeof(CUBE_POS_VERTICES) / sizeof(XrVector3f);
        XrVector3f *vertices = const_cast<XrVector3f *>(&CUBE_POS_VERTICES[0]);
        uint32_t *indices = const_cast<uint32_t *>(&CUBE_INDICES[0]);
        auto indexCnt = sizeof(CUBE_INDICES) / sizeof(uint32_t);
        meshData->SetVerticesAndIndices(vertices, vertexCnt, indices, indexCnt);
    }
}  // namespace PVRSampleFW
