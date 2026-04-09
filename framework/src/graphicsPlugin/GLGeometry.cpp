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

#include "GLGeometry.h"
#include "LogUtils.h"

namespace PVRSampleFW {
    GLGeometry::GLGeometry() : vao_id_(0), vbo_id_(0), ebo_id_(0), draw_index_count_(0) {
    }

    GLGeometry::~GLGeometry() {
        if (vao_id_ != 0) {
            glDeleteVertexArrays(1, &vao_id_);
            vao_id_ = 0;
        }

        if (vbo_id_ != 0) {
            glDeleteBuffers(1, &vbo_id_);
        }

        if (ebo_id_ != 0) {
            glDeleteBuffers(1, &ebo_id_);
        }

        draw_index_count_ = 0;
    }

    void GLGeometry::Initialize(const GLProgramAttribute *pAttrib, int nAttrib, const uint32_t *pIndices, int nIndices,
                                const void *pVertexData, int bufferSize, unsigned int primitiveType) {
        glGenVertexArrays(1, &vao_id_);
        glBindVertexArray(vao_id_);

        glGenBuffers(1, &vbo_id_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id_);
        glBufferData(GL_ARRAY_BUFFER, bufferSize, pVertexData, GL_STATIC_DRAW);

        glGenBuffers(1, &ebo_id_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, nIndices, pIndices, GL_STATIC_DRAW);

        GLint vertexAttribIndex = 0;
        for (int i = 0; i < nAttrib; i++) {
            glEnableVertexAttribArray(pAttrib[i].index);
            glVertexAttribPointer(pAttrib[i].index, pAttrib[i].size, pAttrib[i].type, pAttrib[i].normalized,
                                  pAttrib[i].stride, reinterpret_cast<void *>((pAttrib[i].offset)));
        }

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        draw_index_count_ = nIndices / sizeof(uint32_t);
        PLOGD("GLGeometry::Initialize draw_index_count_=%d", draw_index_count_);

        if (primitiveType >= GL_POINTS && primitiveType <= GL_TRIANGLE_FAN) {
            draw_primitive_type_ = primitiveType;
        } else {
            PLOGE("GLGeometry::Initialize() error primitiveType=%d", primitiveType);
        }
    }

    void GLGeometry::Submit() const {
        if (vao_id_ == 0) {
            PLOGE("GLGeometry::Submit() error vao_id_=%d", vao_id_);
            return;
        }
        glBindVertexArray(vao_id_);
        glDrawElements(draw_primitive_type_, draw_index_count_, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
}  // namespace PVRSampleFW
