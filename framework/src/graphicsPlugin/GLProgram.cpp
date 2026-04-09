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

#include "GLProgram.h"
#include "LogUtils.h"

namespace PVRSampleFW {
    GLProgram::GLProgram() : program_id_(0) {
        gl_uniform_info_map_.clear();
    }

    GLProgram::~GLProgram() {
        gl_uniform_info_map_.clear();
        if (program_id_ != 0) {
            glDeleteProgram(program_id_);
        }
    }

    void GLProgram::CreateProgram(const char *vertString, const char *fragString, const GLProgramAttribute *pAttrib,
                                  uint32_t nAttrib) {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertString, nullptr);
        glCompileShader(vertexShader);
        CheckShader(vertexShader);

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragString, nullptr);
        glCompileShader(fragmentShader);
        CheckShader(fragmentShader);

        program_id_ = glCreateProgram();
        glAttachShader(program_id_, vertexShader);
        glAttachShader(program_id_, fragmentShader);
        for (unsigned int i = 0; i < nAttrib; i++) {
            int index = pAttrib[i].index;
            glBindAttribLocation(program_id_, default_gl_attributes_[index].location, default_gl_attributes_[i].name);
        }
        glLinkProgram(program_id_);
        CheckProgram(program_id_);

        InitUniformInfo();

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void GLProgram::CreateProgram(const char *vertString, const char *geomString, const char *fragString,
                                  const GLProgramAttribute *pAttrib, uint32_t nAttrib) {
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertString, nullptr);
        glCompileShader(vertexShader);
        CheckShader(vertexShader);

        GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometryShader, 1, &geomString, nullptr);
        glCompileShader(geometryShader);
        CheckShader(geometryShader);

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragString, nullptr);
        glCompileShader(fragmentShader);
        CheckShader(fragmentShader);

        program_id_ = glCreateProgram();
        glAttachShader(program_id_, vertexShader);
        glAttachShader(program_id_, geometryShader);
        glAttachShader(program_id_, fragmentShader);
        for (unsigned int i = 0; i < nAttrib; i++) {
            int index = pAttrib[i].index;
            glBindAttribLocation(program_id_, default_gl_attributes_[index].location, default_gl_attributes_[i].name);
        }
        glLinkProgram(program_id_);
        CheckProgram(program_id_);
        InitUniformInfo();
        glDeleteShader(vertexShader);
        glDeleteShader(geometryShader);
        glDeleteShader(fragmentShader);
    }

    void GLProgram::Bind() {
        glUseProgram(program_id_);
    }

    void GLProgram::UnBind() {
        glUseProgram(0);
    }

    void GLProgram::SetUniformMat4(const char *name, const XrMatrix4x4f &matrix) {
        auto it = gl_uniform_info_map_.find(name);
        if (it != gl_uniform_info_map_.end()) {
            glUniformMatrix4fv(it->second.location, 1, GL_FALSE, reinterpret_cast<const GLfloat *>(&matrix));
        } else {
            PLOGW("GLProgram::SetUniformMat4 unknown uniform=%s", name);
        }
    }

    void GLProgram::SetUniformVec3(const char *name, const XrVector3f &vec3) {
        auto it = gl_uniform_info_map_.find(name);
        if (it != gl_uniform_info_map_.end()) {
            glUniform3f(it->second.location, vec3.x, vec3.y, vec3.z);
        } else {
            PLOGW("GLProgram::SetUniformVec3 unknown uniform=%s", name);
        }
    }

    void GLProgram::SetUniformVec2(const char *name, const XrVector2f &vec2) {
        auto it = gl_uniform_info_map_.find(name);
        if (it != gl_uniform_info_map_.end()) {
            glUniform2f(it->second.location, vec2.x, vec2.y);
        } else {
            PLOGW("GLProgram::SetUniformVec3 unknown uniform=%s", name);
        }
    }

    void GLProgram::SetUniformSampler(const char *name, unsigned int samplerId, unsigned int samplerType) {
        auto it = gl_uniform_info_map_.find(name);
        if (it != gl_uniform_info_map_.end()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(samplerType, samplerId);
            glUniform1i(it->second.location, 0);
        } else {
            PLOGW("GLProgram::SetUniformSampler unknown uniform=%s", name);
        }
    }

    void GLProgram::CheckShader(GLuint shader) {
        GLint result = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE) {
            GLchar msg[4096] = {};
            GLsizei length;
            glGetShaderInfoLog(shader, sizeof(msg), &length, msg);
            PLOGE("GLProgram: Compile shader failed: %s", msg);
        }
    }

    void GLProgram::CheckProgram(GLuint glProgram) {
        GLint result = 0;
        glGetProgramiv(glProgram, GL_LINK_STATUS, &result);
        if (result == GL_FALSE) {
            GLchar msg[4096] = {};
            GLsizei length;
            glGetProgramInfoLog(glProgram, sizeof(msg), &length, msg);
            PLOGE("GLProgram: Link program failed: %s", msg);
        }
    }

    bool GLProgram::InitUniformInfo() {
        GLint activeUniforms;
        glGetProgramiv(program_id_, GL_ACTIVE_UNIFORMS, &activeUniforms);
        GLint maxLength;
        glGetProgramiv(program_id_, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);
        char nameBuffer[MAX_UNIFORM_NAME_LENGTH];
        memset(nameBuffer, 0, maxLength);

        for (int i = 0; i < activeUniforms; i++) {
            int uniformNameLength = 0;
            int uniformSize = 0;
            GLenum uniformType;
            glGetActiveUniform(program_id_, i, maxLength, &uniformNameLength, &uniformSize, &uniformType,
                               &nameBuffer[0]);
            if (maxLength < uniformNameLength || uniformNameLength == 0) {
                PLOGE("uniform name length is out of range %d", uniformNameLength);
                return false;
            }
            GLint location = glGetUniformLocation(program_id_, nameBuffer);

            GLUniformInfo uniform;
            uniform.location = location;
            uniform.type = uniformType;
            uniform.name_length = uniformNameLength;
            strncpy(uniform.name, nameBuffer, uniformNameLength);
            gl_uniform_info_map_[nameBuffer] = uniform;
            PLOGD("GLProgram::InitUniformInfo: nameBuffer=%s, location=%d, type=%d", nameBuffer, location, uniformType);
        }

        return true;
    }
}  // namespace PVRSampleFW
