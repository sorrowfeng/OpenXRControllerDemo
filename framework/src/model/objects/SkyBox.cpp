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

#include "SkyBox.h"
#include "LogUtils.h"
#include "stb_image.h"

namespace PVRSampleFW {
    void SkyBox::SetFrontTexture(const std::vector<uint8_t> &buffer) {
        // check front texture scope
        if (texture_scope_.front.first != -1 && texture_scope_.front.second != -1) {
            // overwrite texture_buffer_
            PLOGW("SkyBox::SetFrontTexture overwrite texture_buffer_ occurs");
            auto start = texture_scope_.front.first;
            auto end = texture_scope_.front.first + buffer.size();
            texture_buffer_.insert(texture_buffer_.begin() + texture_scope_.front.first, buffer.begin(), buffer.end());
            texture_scope_.front = {start, end};
            return;
        } else {
            // transfer buffer to texture_buffer_ and record scope
            int start = texture_buffer_.size();
            texture_buffer_.insert(texture_buffer_.end(), buffer.begin(), buffer.end());
            int end = texture_buffer_.size();
            texture_scope_.front = {start, end};
        }
    }

    void SkyBox::SetBackTexture(const std::vector<uint8_t> &buffer) {
        // check back texture scope
        if (texture_scope_.back.first != -1 && texture_scope_.back.second != -1) {
            // overwrite texture_buffer_
            PLOGW("SkyBox::SetBackTexture overwrite texture_buffer_ occurs");
            auto start = texture_scope_.back.first;
            auto end = texture_scope_.back.first + buffer.size();
            texture_buffer_.insert(texture_buffer_.begin() + texture_scope_.back.first, buffer.begin(), buffer.end());
            texture_scope_.back = {start, end};
            return;
        } else {
            // transfer buffer to texture_buffer_ and record scope
            int start = texture_buffer_.size();
            texture_buffer_.insert(texture_buffer_.end(), buffer.begin(), buffer.end());
            int end = texture_buffer_.size();
            texture_scope_.back = {start, end};
        }
    }

    void SkyBox::SetLeftTexture(const std::vector<uint8_t> &buffer) {
        // check left texture scope
        if (texture_scope_.left.first != -1 && texture_scope_.left.second != -1) {
            // overwrite texture_buffer_
            PLOGW("SkyBox::SetLeftTexture overwrite texture_buffer_ occurs");
            auto start = texture_scope_.left.first;
            auto end = texture_scope_.left.first + buffer.size();
            texture_buffer_.insert(texture_buffer_.begin() + texture_scope_.left.first, buffer.begin(), buffer.end());
            texture_scope_.left = {start, end};
            return;
        } else {
            // transfer buffer to texture_buffer_ and record scope
            int start = texture_buffer_.size();
            texture_buffer_.insert(texture_buffer_.end(), buffer.begin(), buffer.end());
            int end = texture_buffer_.size();
            texture_scope_.left = {start, end};
        }
    }

    void SkyBox::SetRightTexture(const std::vector<uint8_t> &buffer) {
        // check right texture scope
        if (texture_scope_.right.first != -1 && texture_scope_.right.second != -1) {
            // overwrite texture_buffer_
            PLOGW("SkyBox::SetRightTexture overwrite texture_buffer_ occurs");
            auto start = texture_scope_.right.first;
            auto end = texture_scope_.right.first + buffer.size();
            texture_buffer_.insert(texture_buffer_.begin() + texture_scope_.right.first, buffer.begin(), buffer.end());
            texture_scope_.right = {start, end};
            return;
        } else {
            // transfer buffer to texture_buffer_ and record scope
            int start = texture_buffer_.size();
            texture_buffer_.insert(texture_buffer_.end(), buffer.begin(), buffer.end());
            int end = texture_buffer_.size();
            texture_scope_.right = {start, end};
        }
    }

    void SkyBox::SetTopTexture(const std::vector<uint8_t> &buffer) {
        // check top texture scope
        if (texture_scope_.top.first != -1 && texture_scope_.top.second != -1) {
            // overwrite texture_buffer_
            PLOGW("SkyBox::SetTopTexture overwrite texture_buffer_ occurs");
            auto start = texture_scope_.top.first;
            auto end = texture_scope_.top.first + buffer.size();
            texture_buffer_.insert(texture_buffer_.begin() + texture_scope_.top.first, buffer.begin(), buffer.end());
            texture_scope_.top = {start, end};
            return;
        } else {
            // transfer buffer to texture_buffer_ and record scope
            int start = texture_buffer_.size();
            texture_buffer_.insert(texture_buffer_.end(), buffer.begin(), buffer.end());
            int end = texture_buffer_.size();
            texture_scope_.top = {start, end};
        }
    }

    void SkyBox::SetBottomTexture(const std::vector<uint8_t> &buffer) {
        // check bottom texture scope
        if (texture_scope_.bottom.first != -1 && texture_scope_.bottom.second != -1) {
            // overwrite texture_buffer_
            PLOGW("SkyBox::SetBottomTexture overwrite texture_buffer_ occurs");
            auto start = texture_scope_.bottom.first;
            auto end = texture_scope_.bottom.first + buffer.size();
            texture_buffer_.insert(texture_buffer_.begin() + texture_scope_.bottom.first, buffer.begin(), buffer.end());
            texture_scope_.bottom = {start, end};
            return;
        } else {
            // transfer buffer to texture_buffer_ and record scope
            int start = texture_buffer_.size();
            texture_buffer_.insert(texture_buffer_.end(), buffer.begin(), buffer.end());
            int end = texture_buffer_.size();
            texture_scope_.bottom = {start, end};
        }
    }

    std::vector<uint8_t> SkyBox::GetFrontTexture() {
        // get the front texture scope
        auto start = texture_scope_.front.first;
        auto end = texture_scope_.front.second;
        // check scope
        if (start == -1 || end == -1) {
            PLOGW("SkyBox::GetFrontTexture scope error");
            return {};
        }
        // get the front texture
        std::vector<uint8_t> frontTexture(texture_buffer_.begin() + start, texture_buffer_.begin() + end);
        return frontTexture;
    }

    std::vector<uint8_t> SkyBox::GetBackTexture() {
        // get the back texture scope
        auto start = texture_scope_.back.first;
        auto end = texture_scope_.back.second;
        // check scope
        if (start == -1 || end == -1) {
            PLOGW("SkyBox::GetBackTexture scope error");
            return {};
        }
        // get the back texture
        std::vector<uint8_t> backTexture(texture_buffer_.begin() + start, texture_buffer_.begin() + end);
        return backTexture;
    }

    std::vector<uint8_t> SkyBox::GetLeftTexture() {
        // get the left texture scope
        auto start = texture_scope_.left.first;
        auto end = texture_scope_.left.second;
        // check scope
        if (start == -1 || end == -1) {
            PLOGW("SkyBox::GetLeftTexture scope error");
            return {};
        }
        // get the left texture
        std::vector<uint8_t> leftTexture(texture_buffer_.begin() + start, texture_buffer_.begin() + end);
        return leftTexture;
    }

    std::vector<uint8_t> SkyBox::GetRightTexture() {
        // get the right texture scope
        auto start = texture_scope_.right.first;
        auto end = texture_scope_.right.second;
        // check scope
        if (start == -1 || end == -1) {
            PLOGW("SkyBox::GetRightTexture scope error");
            return {};
        }
        // get the right texture
        std::vector<uint8_t> rightTexture(texture_buffer_.begin() + start, texture_buffer_.begin() + end);
        return rightTexture;
    }

    std::vector<uint8_t> SkyBox::GetTopTexture() {
        // get the top texture scope
        auto start = texture_scope_.top.first;
        auto end = texture_scope_.top.second;
        // check scope
        if (start == -1 || end == -1) {
            PLOGW("SkyBox::GetTopTexture scope error");
            return {};
        }
        // get the top texture
        std::vector<uint8_t> topTexture(texture_buffer_.begin() + start, texture_buffer_.begin() + end);
        return topTexture;
    }

    std::vector<uint8_t> SkyBox::GetBottomTexture() {
        // get the bottom texture scope
        auto start = texture_scope_.bottom.first;
        auto end = texture_scope_.bottom.second;
        // check scope
        if (start == -1 || end == -1) {
            PLOGW("SkyBox::GetBottomTexture scope error");
            return {};
        }
        // get the bottom texture
        std::vector<uint8_t> bottomTexture(texture_buffer_.begin() + start, texture_buffer_.begin() + end);
        return bottomTexture;
    }

    void SkyBox::GLLoadCubeMapTexture() {
        // check texture_buffer_
        if (GetFrontTexture().empty() || GetBackTexture().empty() || GetLeftTexture().empty() ||
            GetRightTexture().empty() || GetTopTexture().empty() || GetBottomTexture().empty()) {
            PLOGW("SkyBox::GLLoadCubeMapTexture texture_buffer_ error for cube map texture buffers invalid");
            return;
        }

        glGenTextures(1, &color_texture_id_);
        glBindTexture(GL_TEXTURE_CUBE_MAP, color_texture_id_);

        int width, height, nrChannels;
        // front
        auto frontVec = GetFrontTexture();
        unsigned char *front =
                stbi_load_from_memory(frontVec.data(), frontVec.size(), &width, &height, &nrChannels, STBI_default);

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, front);
        stbi_image_free(front);
        // back
        auto backVec = GetBackTexture();
        unsigned char *back =
                stbi_load_from_memory(backVec.data(), backVec.size(), &width, &height, &nrChannels, STBI_default);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, back);
        stbi_image_free(back);
        // right
        auto rightVec = GetRightTexture();
        unsigned char *right =
                stbi_load_from_memory(rightVec.data(), rightVec.size(), &width, &height, &nrChannels, STBI_default);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, right);
        stbi_image_free(right);
        // left
        auto leftVec = GetLeftTexture();
        unsigned char *left =
                stbi_load_from_memory(leftVec.data(), leftVec.size(), &width, &height, &nrChannels, STBI_default);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, left);
        stbi_image_free(left);
        // top
        auto topVec = GetTopTexture();
        unsigned char *top =
                stbi_load_from_memory(topVec.data(), topVec.size(), &width, &height, &nrChannels, STBI_default);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, top);
        stbi_image_free(top);
        // bottom
        auto bottomVec = GetBottomTexture();
        unsigned char *bottom =
                stbi_load_from_memory(bottomVec.data(), bottomVec.size(), &width, &height, &nrChannels, STBI_default);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, bottom);
        stbi_image_free(bottom);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    SkyBox::SkyBox(const XrPosef &pose_, const XrVector3f &scale_) : Object() {
        texture_scope_ = SkyBoxTextureScope{
                .front = {-1, -1},
                .back = {-1, -1},
                .left = {-1, -1},
                .right = {-1, -1},
                .top = {-1, -1},
                .bottom = {-1, -1},
        };
        type_ = OBJECT_TYPE_SKYBOX;
        gl_program_type_ = GL_PROGRAM_TYPE_SAMPLER_CUBE;
        gl_geometry_type_ = GL_GEOMETRY_TYPE_POS_SAMPLERCUBE_SKYBOX;
        this->pose_ = pose_;
        this->scale_ = scale_;
        // Build Object
        BuildObject();
    }

    void SkyBox::BuildObject() {
        // vertex buffer
        auto vertexSize = sizeof(SKY_BOX_VERTEXES);
        vertex_buffer_.resize(vertexSize / sizeof(float));
        std::memcpy(vertex_buffer_.data(), SKY_BOX_VERTEXES, vertexSize);

        // index buffer
        auto indexSize = sizeof(SKY_BOX_INDICES);
        index_buffer_.resize(indexSize / sizeof(uint32_t));
        std::memcpy(index_buffer_.data(), SKY_BOX_INDICES, indexSize);

        // Draw mode
        SetDrawMode(DRAW_MODE_TRIANGLES);

        // Set Draw using array
        SetDrawUsingArrays(true);
    }
}  // namespace PVRSampleFW
