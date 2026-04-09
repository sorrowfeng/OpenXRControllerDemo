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

#ifndef PICONATIVEOPENXRSAMPLES_MESH_H
#define PICONATIVEOPENXRSAMPLES_MESH_H

#include "Object.h"

namespace PVRSampleFW {

    struct MeshVertex {
        XrVector3f position;
        XrColor4f color;
    };

    /**
     * A object that describe a shape with triangle primitives.
     *
     * @note: Please make sure that the vertices and indices
     * you pass in when building this object are  multiples
     * of 3 to ensure that they can correctly describe the
     * expected triangles.
     */
    class Mesh : public Object {
    public:
        Mesh() : Object() {
            type_ = OBJECT_TYPE_MESH;
            gl_program_type_ = GL_PROGRAM_TYPE_COLOR;
            gl_geometry_type_ = GL_GEOMETRY_TYPE_POS_COLOR_MESH;
        }

        Mesh(const XrPosef& p, const XrVector3f& s) : Object() {
            type_ = OBJECT_TYPE_MESH;
            gl_program_type_ = GL_PROGRAM_TYPE_COLOR;
            gl_geometry_type_ = GL_GEOMETRY_TYPE_POS_COLOR_MESH;
            this->pose_ = p;
            this->scale_ = s;
        }

        /**
         * Build a mesh with vertices, indices and color.
         *
         * @param verticesArray vertices, in model coordinate
         * @param vertexCnt number of vertices
         * @param indicesArray indices
         * @param indexCnt number of indices
         * @param color color of vertex
         * @return true: success, false: failed
         */
        bool BuildObject(XrVector3f* verticesArray, uint32_t vertexCnt, uint32_t* indicesArray, uint32_t indexCnt,
                         const XrColor4f& color);

        /**
         * Build a mesh with vertices, indices and color.
         *
         * @param vertices vertices, in model coordinate
         * @param indices indices
         * @param color color of vertex
         * @return true: success, false:failed
         */
        bool BuildObject(const std::vector<XrVector3f>& vertices, const std::vector<uint32_t>& indices,
                         const XrColor4f& color);

        /**
         * Build a mesh with vertex attribute array, indices
         *
         * @param verticesArray vertex attribute array, pos + color
         * @param vertexCnt number of vertices
         * @param indicesArray indices
         * @param indexCnt number of indices
         * @return true: success, false:failed
         */
        bool BuildObject(MeshVertex* verticesArray, uint32_t vertexCnt, uint32_t* indicesArray, uint32_t indexCnt);

        /**
         * Build a mesh with vertex attribute vector, indices vector
         *
         * @param vertices vertex attribute vector, pos + color
         * @param indices indices
         * @return true: success, false:failed
         */
        bool BuildObject(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices);

        /**
         * Only mesh objects that pass the build are considered valid.
         *
         * @return true is valid, false is invalid
         */
        bool IsValid() const {
            return is_valid_;
        }

    private:
        bool is_valid_{false};
    };
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_MESH_H
