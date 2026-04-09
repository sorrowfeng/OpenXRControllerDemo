// Copyright (c) 2017-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

/*
 * Copyright 2024 - 2025 PICO.
 *
 * This code is a PICO modified version from OpenXR SDK distribution by The Khronos Group Inc.
 *
 * */

#ifndef PICONATIVEOPENXRSAMPLES_OPENGLESGRAPHICSPLUGIN_H
#define PICONATIVEOPENXRSAMPLES_OPENGLESGRAPHICSPLUGIN_H

#include "graphicsPlugin/IXrGraphicsPlugin.h"
#include "util/Common.h"
#include "gfxwrapper_opengl.h"
#include "Configurations.h"
#include "GLProgram.h"
#include "GLGeometry.h"
#include "graphicsPlugin/GraphicsConstants.h"
#include "SwapchainImageData.h"
#include "OpenGL/GLResources.h"
#include <list>
#include <map>
#include <openxr/openxr_platform.h>

#define GL(glcmd)                                                                       \
    {                                                                                   \
        GLint err = glGetError();                                                       \
        if (err != GL_NO_ERROR) {                                                       \
            PLOGE("GLES error=%d, %s:%d", err, __FUNCTION__, __LINE__);                 \
        }                                                                               \
        glcmd;                                                                          \
        err = glGetError();                                                             \
        if (err != GL_NO_ERROR) {                                                       \
            PLOGE("GLES error=%d, cmd=%s, %s:%d", err, #glcmd, __FUNCTION__, __LINE__); \
        }                                                                               \
    }

namespace PVRSampleFW {
    struct OpenGLESFallbackDepthTexture {
    public:
        OpenGLESFallbackDepthTexture() = default;
        ~OpenGLESFallbackDepthTexture() {
            Reset();
        }
        void Reset() {
            if (Allocated()) {

                GL(glDeleteTextures(1, &textures_));
            }
            textures_ = 0;
            xrImage_.image = 0;
        }
        bool Allocated() const {
            return textures_ != 0;
        }

        void Allocate(GLuint width, GLuint height, uint32_t arraySize) {
            Reset();
            const bool isArray = arraySize > 1;
            GLenum target = isArray ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
            GL(glGenTextures(1, &textures_));
            GL(glBindTexture(target, textures_));
            GL(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
            GL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            GL(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            GL(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
            if (isArray) {
                GL(glTexImage3D(target, 0, GL_DEPTH_COMPONENT24, width, height, arraySize, 0, GL_DEPTH_COMPONENT,
                                GL_UNSIGNED_INT, nullptr));
            } else {
                GL(glTexImage2D(target, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,
                                nullptr));
            }
            xrImage_.image = textures_;
        }
        const XrSwapchainImageOpenGLESKHR& GetTexture() const {
            return xrImage_;
        }

    private:
        uint32_t textures_{0};
        XrSwapchainImageOpenGLESKHR xrImage_{XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR, NULL, 0};
    };

    class OpenGLESSwapchainImageData : public SwapchainImageDataBase<XrSwapchainImageOpenGLESKHR> {
    public:
        OpenGLESSwapchainImageData(uint32_t capacity, const XrSwapchainCreateInfo& createInfo)
            : SwapchainImageDataBase(XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR, capacity, createInfo)
            , internal_depth_textures_(capacity) {
        }

        OpenGLESSwapchainImageData(uint32_t capacity, const XrSwapchainCreateInfo& createInfo,
                                   XrSwapchain depthSwapchain, const XrSwapchainCreateInfo& depthCreateInfo)
            : SwapchainImageDataBase(XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR, capacity, createInfo, depthSwapchain,
                                     depthCreateInfo) {
        }

    protected:
        const XrSwapchainImageOpenGLESKHR& GetFallbackDepthSwapchainImage(uint32_t i) override {

            if (!internal_depth_textures_[i].Allocated()) {
                internal_depth_textures_[i].Allocate(this->Width(), this->Height(), this->ArraySize());
            }

            return internal_depth_textures_[i].GetTexture();
        }

    private:
        std::vector<OpenGLESFallbackDepthTexture> internal_depth_textures_;
    };

    class OpenGLESGraphicsPlugin : public IXrGraphicsPlugin {
    public:
        explicit OpenGLESGraphicsPlugin(const std::shared_ptr<Configurations>& config)
            : clear_color_(config->GetBackgroundClearColor()) {
        }

        OpenGLESGraphicsPlugin(const OpenGLESGraphicsPlugin&) = delete;
        OpenGLESGraphicsPlugin& operator=(const OpenGLESGraphicsPlugin&) = delete;
        OpenGLESGraphicsPlugin(OpenGLESGraphicsPlugin&&) = delete;
        OpenGLESGraphicsPlugin& operator=(OpenGLESGraphicsPlugin&&) = delete;

        ~OpenGLESGraphicsPlugin() override;

        std::string DescribeGraphics() const override {
            return std::string("OpenGLES");
        }

        std::vector<std::string> GetInstanceExtensionsRequiredByGraphics() const override;

        void InitializeGraphicsDevice(XrInstance instance, XrSystemId systemId, IXrProgram* program) override;

        void ShutdownGraphicsDevice() override;

        // TODO: SwapchainImageData pattern functions, not ready
        void ClearSwapchainCache() override;

        void Flush() override;

        // TODO: SwapchainImageData pattern functions, not ready
        void CopyRGBAImage(const XrSwapchainImageBaseHeader* /*swapchainImage*/, uint32_t /*arraySlice*/,
                           const Conformance::RGBAImage& /*image*/) override;

        void RenderProjectionView(const XrCompositionLayerProjectionView& layerView,
                                  const XrSwapchainImageBaseHeader* swapchainImage, int64_t swapchainFormat,
                                  const std::vector<Scene>& scenes) override;

        int64_t SelectColorSwapchainFormat(const std::vector<int64_t>& runtimeFormats) const override;

        int64_t SelectDepthSwapchainFormat(const std::vector<int64_t>& runtimeFormats) const override;

        int64_t SelectMotionVectorSwapchainFormat(const std::vector<int64_t>& runtimeFormats) const override;

        std::vector<XrSwapchainImageBaseHeader*>
        AllocateSwapchainImageStructs(uint32_t capacity, const XrSwapchainCreateInfo& swapchainCreateInfo) override;

        ISwapchainImageData* AllocateSwapchainImageData(size_t size,
                                                        const XrSwapchainCreateInfo& swapchainCreateInfo) override;

        ISwapchainImageData* AllocateSwapchainImageDataWithDepthSwapchain(
                size_t size, const XrSwapchainCreateInfo& colorSwapchainCreateInfo, XrSwapchain depthSwapchain,
                const XrSwapchainCreateInfo& depthSwapchainCreateInfo) override;

        void ClearImageSlice(const XrSwapchainImageBaseHeader* colorSwapchainImage, uint32_t imageArrayIndex,
                             XrColor4f color) override;

        void ClearImageSlice(const XrSwapchainImageBaseHeader* colorSwapchainImage,
                             uint32_t imageArrayIndex = 0) override;

        const XrBaseInStructure* GetGraphicsBinding() const override;

        void UpdateConfigurationsAtGraphics(const std::shared_ptr<struct Configurations>& config) override;

        void SetBackgroundColor(std::array<float, 4> color) override;

        int64_t GetSRGBA8Format() const override;

        Pbr::IGltfBuilder* GetPbrResources() override;

    public:
        GLGeometry* GetGLGeometry(uint32_t type);

    private:
        virtual void InitializeGraphicsResources();

        void CreateGLGeometries(const Object& object);

    private:
        void DebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                  const GLchar* message);

        uint32_t GetDepthTexture(uint32_t colorTexture);

    private:
        IXrProgram* xr_program_;
        ksGpuWindow window_{};
        GLint context_api_major_version_{0};
#ifdef XR_USE_PLATFORM_ANDROID
        XrGraphicsBindingOpenGLESAndroidKHR graphics_binding_{XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
#endif
        std::list<std::vector<XrSwapchainImageOpenGLESKHR>> swapchain_image_buffers_;
        SwapchainImageDataMap<OpenGLESSwapchainImageData> swapchain_image_data_map_;
        GLuint swapchain_framebuffer_{0};

        // Map color buffer to associated depth buffer. This map is populated on demand.
        std::map<uint32_t, uint32_t> color_to_depth_map_;
        std::array<float, 4> clear_color_;

        // render resource
        std::map<uint32_t, GLProgram*> gl_program_map_;
        std::map<uint32_t, GLGeometry*> gl_geometry_map_;
        std::unique_ptr<Pbr::GLResources> pbr_resources_;

        static constexpr int NUM_POS_COLOR_VERT_ATTRIB{2};
        const GLProgramAttribute pos_color_vert_attrib_[NUM_POS_COLOR_VERT_ATTRIB] = {
                // Index    Size    Type        Normalized  Stride                  Offset
                {kPosition, 3, GL_FLOAT, false, sizeof(GraphicsConstants::Vertex), 0},
                {kColor, 4, GL_FLOAT, false, sizeof(GraphicsConstants::Vertex), sizeof(XrVector3f)},
        };
        static constexpr int NUM_POS_VERT_ATTRIB{1};
        const GLProgramAttribute pos_vert_attrib_[NUM_POS_VERT_ATTRIB] = {
                // Index        Size    Type        Normalized  Stride                  Offset
                {kPosition, 3, GL_FLOAT, false, sizeof(XrVector3f), 0}};
    };
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_OPENGLESGRAPHICSPLUGIN_H
