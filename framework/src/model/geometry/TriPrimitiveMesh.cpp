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

#include "TriPrimitiveMesh.h"
#include "LogUtils.h"

namespace PVRSampleFW {
    namespace Geometry {
        TriPrimitiveMesh::TriPrimitiveMesh(const std::vector<XrVector3f> &vertices,
                                           const std::vector<uint32_t> &indices) {
            indices_.resize(indices.size());
            std::memcpy(indices_.data(), indices.data(), sizeof(uint32_t) * indices.size());
            vertices_.resize(vertices.size());
            std::memcpy(vertices_.data(), vertices.data(), sizeof(XrVector3f) * vertices.size());

            // set aabb
            MinMaxAABB aabb;
            aabb.Init();
            for (auto &vertex : vertices) {
                aabb.Encapsulate(vertex);
            }
            aabb_ = AABB(aabb);
        }

        TriPrimitiveMesh::TriPrimitiveMesh(XrVector3f *verticesArray, uint32_t vertexCnt, uint32_t *indicesArray,
                                           uint32_t indexCnt) {
            if (indexCnt % 3 != 0) {
                PLOGE("TriPrimitiveMesh::TriPrimitiveMesh failed for invalid size of vertices and indices");
                return;
            }
            vertices_.resize(vertexCnt);
            indices_.resize(indexCnt);
            std::memcpy(vertices_.data(), verticesArray, sizeof(XrVector3f) * vertexCnt);
            std::memcpy(indices_.data(), indicesArray, sizeof(uint32_t) * indexCnt);

            // set aabb
            MinMaxAABB aabb;
            aabb.Init();
            for (auto &vertex : vertices_) {
                aabb.Encapsulate(vertex);
            }
            aabb_ = AABB(aabb);
        }

        void TriPrimitiveMesh::SetVertices(const std::vector<XrVector3f> &vertices) {
            vertices_.resize(vertices.size());
            std::memcpy(vertices_.data(), vertices.data(), sizeof(XrVector3f) * vertices.size());

            // set aabb
            MinMaxAABB aabb;
            aabb.Init();
            for (auto &vertex : vertices_) {
                aabb.Encapsulate(vertex);
            }
            aabb_ = AABB(aabb);
        }

        void TriPrimitiveMesh::SetIndices(std::vector<uint32_t>) {
            indices_.resize(indices_.size());
            std::memcpy(indices_.data(), indices_.data(), sizeof(uint32_t) * indices_.size());
        }

        bool TriPrimitiveMesh::IsValid() const {
            return indices_.size() > 0 && vertices_.size() > 0 && indices_.size() % 3 == 0;
        }

        void TriPrimitiveMesh::SetVerticesAndIndices(XrVector3f *verticesArray, uint32_t vertexCnt,
                                                     uint32_t *indicesArray, uint32_t indexCnt) {
            if (indexCnt % 3 != 0) {
                PLOGE("TriPrimitiveMesh::SetVerticesAndIndices failed for invalid size of vertices and indices");
                return;
            }
            vertices_.resize(vertexCnt);
            indices_.resize(indexCnt);
            std::memcpy(vertices_.data(), verticesArray, sizeof(XrVector3f) * vertexCnt);
            std::memcpy(indices_.data(), indicesArray, sizeof(uint32_t) * indexCnt);

            // set aabb
            MinMaxAABB aabb;
            aabb.Init();
            for (auto &vertex : vertices_) {
                aabb.Encapsulate(vertex);
            }
            aabb_ = AABB(aabb);
        }

        void ScaleTriPrimitiveMesh(const TriPrimitiveMesh &mesh, const XrVector3f &scale, TriPrimitiveMesh *result) {
            auto vertices = mesh.GetVertices();
            for (auto &vertex : vertices) {
                vertex.x *= scale.x;
                vertex.y *= scale.y;
                vertex.z *= scale.z;
            }
            result->SetVertices(vertices);
            auto indices = mesh.GetIndices();
            result->SetIndices(indices);
            /*// scale aabb
            AABB aabb = mesh.GetAABB();
            aabb.Scale(scale);
            result.SetAABB(aabb);*/
        }

        void TransformTriPrimitiveMesh(const TriPrimitiveMesh &mesh, const XrPosef &transform,
                                       TriPrimitiveMesh *result) {
            auto vertices = mesh.GetVertices();
            for (auto &vertex : vertices) {
                XrPosef_TransformVector3f(&vertex, &transform, &vertex);
            }
            result->SetVertices(vertices);
            auto indices = mesh.GetIndices();
            result->SetIndices(indices);
            /*// transform aabb
            AABB aabb = mesh.GetAABB();
            TransformAABB(aabb, transform.position, transform.orientation, aabb);
            result.SetAABB(aabb);*/
        }
    }  // namespace Geometry
}  // namespace PVRSampleFW
