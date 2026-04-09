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

#ifndef PICONATIVEOPENXRSAMPLES_SKYBOX_H
#define PICONATIVEOPENXRSAMPLES_SKYBOX_H

#include "Object.h"
namespace PVRSampleFW {
    constexpr float SKY_BOX_VERTEXES[] = {
            // 位置
            -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
            1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

            -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
            -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

            1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

            -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

            -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
            1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

    constexpr unsigned int SKY_BOX_INDICES[] = {
            0,  1,  2,  3,  4,  5,   // -X
            6,  7,  8,  9,  10, 11,  // +X
            12, 13, 14, 15, 16, 17,  // -Y
            18, 19, 20, 21, 22, 23,  // +Y
            24, 25, 26, 27, 28, 29,  // -Z
            30, 31, 32, 33, 34, 35,  // +Z
    };

    class SkyBox : public Object {
    public:
        struct SkyBoxTextureScope {
            std::pair<int, int> front{-1, -1};
            std::pair<int, int> back{-1, -1};
            std::pair<int, int> left{-1, -1};
            std::pair<int, int> right{-1, -1};
            std::pair<int, int> top{-1, -1};
            std::pair<int, int> bottom{-1, -1};
        };

    public:
        SkyBox() {
            texture_scope_ = SkyBoxTextureScope{
                    .front = {-1, -1},
                    .back = {-1, -1},
                    .left = {-1, -1},
                    .right = {-1, -1},
                    .top = {-1, -1},
                    .bottom = {-1, -1},
            };
            type_ = ObjectType::OBJECT_TYPE_SKYBOX;
            gl_program_type_ = GL_PROGRAM_TYPE_SAMPLER_CUBE;
            gl_geometry_type_ = GL_GEOMETRY_TYPE_POS_SAMPLERCUBE_SKYBOX;
            this->pose_ = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
            this->scale_ = {1.0f, 1.0f, 1.0f};
            BuildObject();
        }

        SkyBox(const XrPosef& pose_, const XrVector3f& scale);

        virtual ~SkyBox() {
        }

        void SetFrontTexture(const std::vector<uint8_t>& buffer);
        void SetBackTexture(const std::vector<uint8_t>& buffer);
        void SetLeftTexture(const std::vector<uint8_t>& buffer);
        void SetRightTexture(const std::vector<uint8_t>& buffer);
        void SetTopTexture(const std::vector<uint8_t>& buffer);
        void SetBottomTexture(const std::vector<uint8_t>& buffer);

        std::vector<uint8_t> GetFrontTexture();
        std::vector<uint8_t> GetBackTexture();
        std::vector<uint8_t> GetLeftTexture();
        std::vector<uint8_t> GetRightTexture();
        std::vector<uint8_t> GetTopTexture();
        std::vector<uint8_t> GetBottomTexture();

        // TODO: temporary
        void GLLoadCubeMapTexture();

    private:
        void BuildObject();

    private:
        SkyBoxTextureScope texture_scope_;
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_SKYBOX_H
