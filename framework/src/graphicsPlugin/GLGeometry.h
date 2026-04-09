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

#ifndef PICONATIVEOPENXRSAMPLES_GLGEOMETRY_H
#define PICONATIVEOPENXRSAMPLES_GLGEOMETRY_H

#include <GLES3/gl3.h>
#include <openxr/openxr.h>
#include "GLProgram.h"

namespace PVRSampleFW {

    struct cubeMeshVert {
        XrVector3f position;
        XrVector3f color;
    };

    struct quadMeshVert {
        XrVector3f position;
        XrVector3f uv;
    };

    /**
     * name: vertex attributes + shaderType
     */
    enum GLGeometryType {
        GL_GEOMETRY_TYPE_POS_COLOR_CUBE = 0,
        GL_GEOMETRY_TYPE_POS_COLOR_TRUNCATED_CONE,
        GL_GEOMETRY_TYPE_POS_COLOR_CARTESIAN_BRANCH,
        GL_GEOMETRY_TYPE_POS_SAMPLER2D_QUAD,
        GL_GEOMETRY_TYPE_POS_SAMPLERCUBE_SKYBOX,
        GL_GEOMETRY_TYPE_POS_COLOR_MESH,
        GL_GEOMETRY_TYPE_PBR_SELF_OWNED,
        GL_GEOMETRY_TYPE_NUM
    };

    class GLGeometry {
    public:
        GLGeometry();

        ~GLGeometry();

        void Initialize(const GLProgramAttribute* pAttrib, int nAttrib, const uint32_t* pIndices, int nIndices,
                        const void* pVertexData, int bufferSize, unsigned int primitiveType);

        void Submit() const;

    private:
        GLuint vao_id_{0};
        GLuint vbo_id_{0};
        GLuint ebo_id_{0};
        GLuint draw_primitive_type_{GL_TRIANGLES};
        int draw_index_count_{0};
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_GLGEOMETRY_H
