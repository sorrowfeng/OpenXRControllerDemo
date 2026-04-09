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

#include "Mesh.h"
#include "TriPrimitiveMesh.h"

namespace PVRSampleFW {
    bool Mesh::BuildObject(XrVector3f* verticesArray, uint32_t vertexCnt, uint32_t* indicesArray, uint32_t indexCnt,
                           const XrColor4f& color) {
        if (vertexCnt <= 0 || indexCnt <= 0 || indexCnt % 3 != 0 || nullptr == verticesArray ||
            nullptr == indicesArray) {
            return false;
        }
        auto vertexSize = sizeof(MeshVertex) * vertexCnt;  // position + color
        vertex_buffer_.resize(vertexSize / sizeof(float));

        // build up mesh vertex attribute
        std::vector<MeshVertex> vertices(vertexCnt);
        for (uint32_t i = 0; i < vertexCnt; i++) {
            vertices[i].position = verticesArray[i];
            vertices[i].color = color;
        }
        std::memcpy(vertex_buffer_.data(), vertices.data(), vertexSize);

        auto indexSize = sizeof(uint32_t) * indexCnt;
        index_buffer_.resize(indexCnt);
        std::memcpy(index_buffer_.data(), indicesArray, indexSize);

        // Draw mode
        SetDrawMode(DRAW_MODE_TRIANGLES);

        // Set Draw using array
        SetDrawUsingArrays(true);

        // set solid
        SetSolid(true);

        // set movable as default
        SetMovable(false);

        // build mesh data
        meshData = std::make_shared<PVRSampleFW::Geometry::TriPrimitiveMesh>();
        meshData->SetVerticesAndIndices(verticesArray, vertexCnt, indicesArray, indexCnt);

        // set valid
        is_valid_ = true;

        // enable wireframe as default
        EnableWireframe(true);
        return true;
    }

    bool Mesh::BuildObject(const std::vector<XrVector3f>& vertices, const std::vector<uint32_t>& indices,
                           const XrColor4f& color) {
        auto verticesCnt = vertices.size();
        auto indicesCnt = indices.size();
        if (verticesCnt <= 0 || indicesCnt <= 0 || indicesCnt % 3 != 0) {
            return false;
        }

        auto vertexSize = sizeof(MeshVertex) * verticesCnt;  // position + color
        vertex_buffer_.resize(vertexSize / sizeof(float));
        // build up mesh vertex attribute
        std::vector<MeshVertex> meshVertices(verticesCnt);
        for (uint32_t i = 0; i < verticesCnt; i++) {
            meshVertices[i].position = vertices[i];
            meshVertices[i].color = color;
        }
        std::memcpy(vertex_buffer_.data(), meshVertices.data(), vertexSize);

        auto indexSize = sizeof(uint32_t) * indicesCnt;
        index_buffer_.resize(indicesCnt);
        std::memcpy(index_buffer_.data(), indices.data(), indexSize);

        // Draw mode
        SetDrawMode(DRAW_MODE_TRIANGLES);
        // Set Draw using array
        SetDrawUsingArrays(true);

        // set solid as default
        SetSolid(true);

        // set movable as default
        SetMovable(false);

        // build mesh data
        meshData = std::make_shared<PVRSampleFW::Geometry::TriPrimitiveMesh>(vertices, indices);

        // set valid
        is_valid_ = true;

        // enable wireframe as default
        EnableWireframe(true);
        return true;
    }

    bool Mesh::BuildObject(MeshVertex* verticesArray, uint32_t vertexCnt, uint32_t* indicesArray, uint32_t indexCnt) {
        if (vertexCnt <= 0 || indexCnt <= 0 || indexCnt % 3 != 0 || nullptr == verticesArray ||
            nullptr == indicesArray) {
            return false;
        }
        auto vertexSize = sizeof(MeshVertex) * vertexCnt;  // position + color
        vertex_buffer_.resize(vertexSize / sizeof(float));
        std::memcpy(vertex_buffer_.data(), verticesArray, vertexSize);
        auto indexSize = sizeof(uint32_t) * indexCnt;
        index_buffer_.resize(indexCnt);
        std::memcpy(index_buffer_.data(), indicesArray, indexSize);

        // Draw mode
        SetDrawMode(DRAW_MODE_TRIANGLES);

        // Set Draw using array
        SetDrawUsingArrays(true);

        // set solid as default
        SetSolid(true);

        // set movable as default
        SetMovable(false);

        // build up vertices and indices
        std::vector<XrVector3f> positions(vertexCnt);
        for (uint32_t i = 0; i < vertexCnt; i++) {
            positions[i] = verticesArray[i].position;
        }

        // build mesh data
        meshData = std::make_shared<PVRSampleFW::Geometry::TriPrimitiveMesh>();
        meshData->SetVerticesAndIndices(positions.data(), vertexCnt, indicesArray, indexCnt);

        // set valid
        is_valid_ = true;

        // enable wireframe as default
        EnableWireframe(true);
        return true;
    }

    bool Mesh::BuildObject(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices) {
        auto verticesCnt = vertices.size();
        auto indicesCnt = indices.size();
        if (verticesCnt <= 0 || indicesCnt <= 0 || indicesCnt % 3 != 0) {
            return false;
        }

        auto vertexSize = sizeof(MeshVertex) * verticesCnt;  // position + color
        vertex_buffer_.resize(vertexSize / sizeof(float));
        std::memcpy(vertex_buffer_.data(), vertices.data(), vertexSize);
        auto indexSize = sizeof(uint32_t) * indicesCnt;
        index_buffer_.resize(indicesCnt);
        std::memcpy(index_buffer_.data(), indices.data(), indexSize);
        // Draw mode
        SetDrawMode(DRAW_MODE_TRIANGLES);
        // Set Draw using array
        SetDrawUsingArrays(true);
        // set solid as default
        SetSolid(true);
        // set movable as default
        SetMovable(false);
        // build up vertices and indices
        std::vector<XrVector3f> positions(verticesCnt);
        for (uint32_t i = 0; i < verticesCnt; i++) {
            positions[i] = vertices[i].position;
        }
        // build mesh data
        meshData = std::make_shared<PVRSampleFW::Geometry::TriPrimitiveMesh>(positions, indices);
        // set valid
        is_valid_ = true;

        // enable wireframe as default
        EnableWireframe(true);
        return true;
    }
}  // namespace PVRSampleFW
