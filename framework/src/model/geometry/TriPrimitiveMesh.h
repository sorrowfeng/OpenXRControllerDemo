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

#ifndef PICONATIVEOPENXRSAMPLES_TRIPRIMITIVEMESH_H
#define PICONATIVEOPENXRSAMPLES_TRIPRIMITIVEMESH_H

#include <vector>
#include "xr_linear.h"
#include "AABB.h"

namespace PVRSampleFW {
    namespace Geometry {
        class TriPrimitiveMesh {
        public:
            TriPrimitiveMesh() {
            }

            TriPrimitiveMesh(const std::vector<XrVector3f>& vertices, const std::vector<uint32_t>& indices);

            TriPrimitiveMesh(XrVector3f* verticesArray, uint32_t vertexCnt, uint32_t* indicesArray, uint32_t indexCnt);

            const std::vector<XrVector3f>& GetVertices() const {
                return vertices_;
            }

            void SetVertices(const std::vector<XrVector3f>& vertices);

            const std::vector<uint32_t>& GetIndices() const {
                return indices_;
            }

            void SetIndices(std::vector<uint32_t>);

            void SetVerticesAndIndices(XrVector3f* verticesArray, uint32_t vertexCnt, uint32_t* indicesArray,
                                       uint32_t indexCnt);

            bool IsValid() const;

            void SetAABB(const AABB& aabb) {
                aabb_ = aabb;
            }

            const AABB& GetAABB() const {
                return aabb_;
            }

        private:
            std::vector<XrVector3f> vertices_;
            std::vector<uint32_t> indices_;
            AABB aabb_;
        };

        void ScaleTriPrimitiveMesh(const TriPrimitiveMesh& aabb, const XrVector3f& scale, TriPrimitiveMesh* result);

        void TransformTriPrimitiveMesh(const TriPrimitiveMesh& aabb, const XrPosef& transform,
                                       TriPrimitiveMesh* result);

    }  // namespace Geometry
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_TRIPRIMITIVEMESH_H
