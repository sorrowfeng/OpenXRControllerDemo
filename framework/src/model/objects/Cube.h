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

#ifndef PICONATIVEOPENXRSAMPLES_CUBE_H
#define PICONATIVEOPENXRSAMPLES_CUBE_H

#include "Object.h"
#include "GLGeometry.h"
#include "GLProgram.h"
#include <algorithm>

namespace PVRSampleFW {
    static constexpr XrColor4f Red{1, 0, 0, 1};
    static constexpr XrColor4f DarkRed{0.25f, 0, 0, 1};
    static constexpr XrColor4f Green{0, 1, 0, 1};
    static constexpr XrColor4f DarkGreen{0, 0.25f, 0, 1};
    static constexpr XrColor4f Blue{0, 0, 1, 1};
    static constexpr XrColor4f DarkBlue{0, 0, 0.25f, 1};

    // Vertices for a 1x1x1 meter cube. (Left/Right, Top/Bottom, Front/Back)
    static constexpr XrVector3f LBB{-0.5f, -0.5f, -0.5f};
    static constexpr XrVector3f LBF{-0.5f, -0.5f, 0.5f};
    static constexpr XrVector3f LTB{-0.5f, 0.5f, -0.5f};
    static constexpr XrVector3f LTF{-0.5f, 0.5f, 0.5f};
    static constexpr XrVector3f RBB{0.5f, -0.5f, -0.5f};
    static constexpr XrVector3f RBF{0.5f, -0.5f, 0.5f};
    static constexpr XrVector3f RTB{0.5f, 0.5f, -0.5f};
    static constexpr XrVector3f RTF{0.5f, 0.5f, 0.5f};
    static constexpr XrVector3f CUBE_POS_VERTICES[] = {
            LTB, LBF, LBB, LTB, LTF, LBF, RTB, RBB, RBF, RTB, RBF, RTF, LBB, LBF, RBF, LBB, RBF, RBB,
            LTB, RTB, RTF, LTB, RTF, LTF, LBB, RBB, RTB, LBB, RTB, LTB, LBF, LTF, RTF, LBF, RTF, RBF,
    };

    static constexpr uint32_t CUBE_INDICES[] = {
            0,  1,  2,  3,  4,  5,   // -X
            6,  7,  8,  9,  10, 11,  // +X
            12, 13, 14, 15, 16, 17,  // -Y
            18, 19, 20, 21, 22, 23,  // +Y
            24, 25, 26, 27, 28, 29,  // -Z
            30, 31, 32, 33, 34, 35,  // +Z
    };

    static constexpr XrColor4f CUBE_COLOR_DATA[] = {
            DarkRed,   DarkRed,   DarkRed,   DarkRed,   DarkRed,   DarkRed,   Red,   Red,   Red,   Red,   Red,   Red,
            DarkGreen, DarkGreen, DarkGreen, DarkGreen, DarkGreen, DarkGreen, Green, Green, Green, Green, Green, Green,
            DarkBlue,  DarkBlue,  DarkBlue,  DarkBlue,  DarkBlue,  DarkBlue,  Blue,  Blue,  Blue,  Blue,  Blue,  Blue,
    };

    struct PosColorVertex {
        XrVector3f position;
        XrColor4f color;
    };

    static constexpr float CUBE_COLOR_DATA_WITH_TRANSPARENCY[] = {
            // Front face (red)
            1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f,
            0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.5f,

            // Right face (green)
            0.0f, 1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f, 1.0f,
            0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.5f,

            // Back face (blue)
            0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 0.0f,
            1.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.5f,

            // Left face (yellow)
            1.0f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f,
            0.0f, 0.5f, 1.0f, 1.0f, 0.0f, 0.5f,

            // Top face (cyan)
            0.0f, 1.0f, 1.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f, 0.0f, 1.0f,
            1.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f,

            // Bottom face (magenta)
            1.0f, 0.0f, 1.0f, 0.5f, 1.0f, 0.0f, 1.0f, 0.5f, 1.0f, 0.0f, 1.0f, 0.5f, 1.0f, 0.0f, 1.0f, 0.5f, 1.0f, 0.0f,
            1.0f, 0.5f, 1.0f, 0.0f, 1.0f, 0.5f};

    static constexpr float CUBE_NORMAL_DATA[] = {
            // Front face
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

            // Right face
            1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

            // Back face
            0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            -1.0f,

            // Left face
            -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
            0.0f,

            // Top face
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

            // Bottom face
            0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,
            0.0f};

    static constexpr float CUBE_TEXTURE_COORDINATE_DATA[] = {
            // Front face
            0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,

            // Right face
            0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,

            // Back face
            0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,

            // Left face
            0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,

            // Top face
            0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,

            // Bottom face
            0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f};

    class Cube : public Object {
    public:
        Cube() : Object() {
            type_ = OBJECT_TYPE_CUBE;
            gl_program_type_ = GL_PROGRAM_TYPE_COLOR;
            gl_geometry_type_ = GL_GEOMETRY_TYPE_POS_COLOR_CUBE;
            // default build
            BuildObjectV2();
            BuildMeshData();
        }
        Cube(const XrPosef& p, const XrVector3f& s) : Object() {
            type_ = OBJECT_TYPE_CUBE;
            gl_program_type_ = GL_PROGRAM_TYPE_COLOR;
            gl_geometry_type_ = GL_GEOMETRY_TYPE_POS_COLOR_CUBE;
            this->pose_ = p;
            this->scale_ = s;
            BuildObjectV2();
            BuildMeshData();
        }

    private:
        /**
         * build：position
         */
        void BuildObjectV1();
        /**
         * build: position + index
         */
        void BuildObjectV2();
        /**
         * build: position + color + normal
         */
        void BuildObjectV3();
        /**
         * build: position + textureCoord + material.texture
         */
        void BuildObjectV4(const std::vector<uint8_t>& textureData);
        /**
         * build: position + textureCoord + material.texture + colorTransparency
         */
        void BuildObjectV5(const std::vector<uint8_t>& textureData);

        void BuildMeshData();
    };
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_CUBE_H
