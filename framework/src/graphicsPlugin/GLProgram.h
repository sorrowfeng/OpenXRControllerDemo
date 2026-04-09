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

#ifndef PICONATIVEOPENXRSAMPLES_GLPROGRAM_H
#define PICONATIVEOPENXRSAMPLES_GLPROGRAM_H

#include <GLES3/gl32.h>
#include <unordered_map>
#include <string>
#include <xr_linear.h>

#define MAX_UNIFORM_NAME_LENGTH 64

namespace PVRSampleFW {

    enum GLProgramType {
        GL_PROGRAM_TYPE_COLOR = 0,
        GL_PROGRAM_TYPE_SAMPLER_2D = 1,
        GL_PROGRAM_TYPE_SAMPLER_CUBE = 2,
        GL_PROGRAM_TYPE_COLOR_WIREFRAME = 3,
        GL_PROGRAM_TYPE_PBR_SELF_OWNED = 4,
        GL_PROGRAM_TYPE_NUM,
    };

    enum GLAttributeLocation {
        kPosition = 0,
        kColor = 1,
        kTexCoordinate0 = 2,
    };

    struct GLAttribute {
        GLAttributeLocation location;
        const char *name;
    };

    struct GLProgramAttribute {
        uint32_t index;
        uint32_t size;
        uint32_t type;
        bool normalized;
        uint32_t stride;
        uint32_t offset;
    };

    struct GLUniformInfo {
        int location;
        GLenum type;
        GLenum texture_unit{0};
        int name_length{0};
        char name[MAX_UNIFORM_NAME_LENGTH];
    };

    class GLProgram {
    public:
        GLProgram();

        ~GLProgram();

        void CreateProgram(const char *vertString, const char *fragString, const GLProgramAttribute *pAttrib,
                           uint32_t nAttrib);

        void CreateProgram(const char *vertString, const char *geomString, const char *fragString,
                           const GLProgramAttribute *pAttrib, uint32_t nAttrib);

        void Bind();

        void UnBind();

        void SetUniformMat4(const char *name, const XrMatrix4x4f &matrix);

        void SetUniformVec3(const char *name, const XrVector3f &vec3);

        void SetUniformVec2(const char *name, const XrVector2f &vec2);

        void SetUniformSampler(const char *name, unsigned int samplerId, unsigned int samplerType);

    private:
        void CheckShader(GLuint shader);

        void CheckProgram(GLuint program);

        bool InitUniformInfo();

    private:
        GLuint program_id_{0};
        std::unordered_map<std::string, GLUniformInfo> gl_uniform_info_map_;

        static constexpr GLAttribute default_gl_attributes_[] = {
                {kPosition, "VertexPos"},
                {kColor, "VertexColor"},
                {kTexCoordinate0, "TexCoords"},
        };
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_GLPROGRAM_H
