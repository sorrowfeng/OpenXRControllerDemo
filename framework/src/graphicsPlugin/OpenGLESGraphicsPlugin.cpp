// Copyright (c) 2017-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

/*
 * Copyright 2024 - 2025 PICO.
 *
 * This code is a PICO modified version from OpenXR SDK distribution by The Khronos Group Inc.
 *
 * */

#include "graphicsPlugin/OpenGLESGraphicsPlugin.h"
#include "util/CheckUtils.h"
#include "graphicsPlugin/GraphicsConstants.h"
#include "xr_linear.h"
#include "SkyBox.h"
#include "Cube.h"
#include "GuiPlane.h"
#include "OpenGL/GLTexture.h"
#include "GltfModel.h"

namespace PVRSampleFW {
    OpenGLESGraphicsPlugin::~OpenGLESGraphicsPlugin() {
        if (swapchain_framebuffer_ != 0) {
            glDeleteFramebuffers(1, &swapchain_framebuffer_);
        }

        for (auto &colorToDepth : color_to_depth_map_) {
            if (colorToDepth.second != 0) {
                glDeleteTextures(1, &colorToDepth.second);
            }
        }
        for (auto &program : gl_program_map_) {
            if (program.second != 0) {
                delete program.second;
            }
        }
        gl_program_map_.clear();
        for (auto &geometry : gl_geometry_map_) {
            if (geometry.second != 0) {
                delete geometry.second;
            }
        }
        gl_geometry_map_.clear();

        ksGpuWindow_Destroy(&window_);
    }
    std::vector<std::string> OpenGLESGraphicsPlugin::GetInstanceExtensionsRequiredByGraphics() const {
        std::vector<std::string> extensions = {XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME};
        return extensions;
    }

    void OpenGLESGraphicsPlugin::InitializeGraphicsDevice(XrInstance instance, XrSystemId systemId,
                                                          IXrProgram *program) {
        if (nullptr == program) {
            PLOGE("program is nullptr");
            return;
        }
        xr_program_ = program;

        // Extension function must be loaded by name
        PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = nullptr;
        CHECK_XRCMD(
                xrGetInstanceProcAddr(instance, "xrGetOpenGLESGraphicsRequirementsKHR",
                                      reinterpret_cast<PFN_xrVoidFunction *>(&pfnGetOpenGLESGraphicsRequirementsKHR)));

        XrGraphicsRequirementsOpenGLESKHR graphicsRequirements = {XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR, nullptr,
                                                                  XR_MAKE_VERSION(3, 1, 0), XR_MAKE_VERSION(3, 2, 0)};
        auto result = CHECK_XRCMD(pfnGetOpenGLESGraphicsRequirementsKHR(instance, systemId, &graphicsRequirements));
        if (XR_FAILED(result)) {
            PLOGE("Unable to get OpenGLESGraphicsRequirementsKHR");
            return;
        }

        // Initialize the gl extensions. Note we have to open a window_.
        ksDriverInstance driverInstance{};
        ksGpuQueueInfo queueInfo{};
        ksGpuSurfaceColorFormat colorFormat{KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8};
        ksGpuSurfaceDepthFormat depthFormat{KS_GPU_SURFACE_DEPTH_FORMAT_D24};
        ksGpuSampleCount sampleCount{KS_GPU_SAMPLE_COUNT_1};
        if (!ksGpuWindow_Create(&window_, &driverInstance, &queueInfo, 0, colorFormat, depthFormat, sampleCount, 640,
                                480, false)) {
            THROW("Unable to create GL context");
        }

        GLint major = 0;
        GLint minor = 0;
        GL(glGetIntegerv(GL_MAJOR_VERSION, &major));
        GL(glGetIntegerv(GL_MINOR_VERSION, &minor));
        auto error = glGetError();
        if (error != GL_NO_ERROR) {
            ShutdownGraphicsDevice();
            PLOGE("Unable to create GL context");
            return;
        }

        const XrVersion desiredApiVersion = XR_MAKE_VERSION(major, minor, 0);
        if (graphicsRequirements.minApiVersionSupported > desiredApiVersion) {
            ShutdownGraphicsDevice();
            THROW("Runtime does not support desired Graphics API and/or version");
        }

        context_api_major_version_ = major;

#if defined(XR_USE_PLATFORM_ANDROID)
        graphics_binding_.display = window_.display;
        graphics_binding_.config = (EGLConfig)0;
        graphics_binding_.context = window_.context.context;
#endif

        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(
                [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message,
                   const void *userParam) {
                    (reinterpret_cast<OpenGLESGraphicsPlugin *>(const_cast<void *>(userParam)))
                            ->DebugMessageCallback(source, type, id, severity, length, message);
                },
                this);

        InitializeGraphicsResources();
    }

    void OpenGLESGraphicsPlugin::DebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                                      GLsizei length, const GLchar *message) {
        (void)source;
        (void)type;
        (void)id;
        (void)severity;
        PLOGV("GLES Debug: %s", std::string(message, 0, length).c_str());
    }

    void OpenGLESGraphicsPlugin::InitializeGraphicsResources() {
        // Color
        auto it = gl_program_map_.find(GL_PROGRAM_TYPE_COLOR);
        if (it == gl_program_map_.end()) {
            GLProgram *glProgram = new GLProgram();
            glProgram->CreateProgram(GraphicsConstants::kSimpleVertexShaderGlsl,
                                     GraphicsConstants::kSimpleFragmentShaderGlsl, pos_color_vert_attrib_,
                                     NUM_POS_COLOR_VERT_ATTRIB);
            gl_program_map_[GL_PROGRAM_TYPE_COLOR] = glProgram;
        }

        // Sampler 2d
        it = gl_program_map_.find(GL_PROGRAM_TYPE_SAMPLER_2D);
        if (it == gl_program_map_.end()) {
            GLProgram *glProgram = new GLProgram();
            glProgram->CreateProgram(GraphicsConstants::KGuiVertexShaderGlsl, GraphicsConstants::kGuiFragmentShaderGlsl,
                                     pos_color_vert_attrib_, NUM_POS_COLOR_VERT_ATTRIB);
            gl_program_map_[GL_PROGRAM_TYPE_SAMPLER_2D] = glProgram;
        }

        // Sampler cube
        it = gl_program_map_.find(GL_PROGRAM_TYPE_SAMPLER_CUBE);
        if (it == gl_program_map_.end()) {
            GLProgram *glProgram = new GLProgram();
            glProgram->CreateProgram(GraphicsConstants::kSkyBoxVertexShaderGlsl,
                                     GraphicsConstants::kSkyBoxFragmentShaderGlsl, pos_vert_attrib_,
                                     NUM_POS_VERT_ATTRIB);
            gl_program_map_[GL_PROGRAM_TYPE_SAMPLER_CUBE] = glProgram;
        }

        // wireframe
        it = gl_program_map_.find(GL_PROGRAM_TYPE_COLOR_WIREFRAME);
        if (it == gl_program_map_.end()) {
            GLProgram *glProgram = new GLProgram();
            glProgram->CreateProgram(GraphicsConstants::kSimpleWireframeVertexShaderGlsl,
                                     GraphicsConstants::kSimpleWireframeGeometryShaderGlsl,
                                     GraphicsConstants::kSimpleWireframeFragmentShaderGlsl, pos_color_vert_attrib_,
                                     NUM_POS_COLOR_VERT_ATTRIB);
            gl_program_map_[GL_PROGRAM_TYPE_COLOR_WIREFRAME] = glProgram;
        }

        glGenFramebuffers(1, &swapchain_framebuffer_);

        /// Create pbr resources
        pbr_resources_ = std::make_unique<Pbr::GLResources>();
        // set the default diffuse light direction and diffuse color.
        pbr_resources_->SetLight({0.0f, 0.7071067811865475f, 0.7071067811865475f}, Pbr::RGB::White);

        // set the default environment map. We use black cube map as default.
        auto blackCubeMap =
                std::make_shared<Pbr::ScopedGLTexture>(Pbr::GLTexture::CreateFlatCubeTexture(Pbr::RGBA::Black, false));
        pbr_resources_->SetEnvironmentMap(blackCubeMap, blackCubeMap);

        std::vector<unsigned char> brdfLutFileData = xr_program_->LoadFileFromAsset("brdf_lut.png");
        auto brdLutResourceView = std::make_shared<Pbr::ScopedGLTexture>(Pbr::GLTexture::LoadTextureImage(
                *pbr_resources_, false, brdfLutFileData.data(), static_cast<uint32_t>(brdfLutFileData.size())));
        pbr_resources_->SetBrdfLut(brdLutResourceView);
    }

    void OpenGLESGraphicsPlugin::ShutdownGraphicsDevice() {
        PLOGI("ShutdownGraphicsDevice");
        /// swapchain_framebuffer_
        if (swapchain_framebuffer_ != 0) {
            GL(glDeleteFramebuffers(1, &swapchain_framebuffer_));
            swapchain_framebuffer_ = 0;
        }

        /// gl_program
        for (auto &program : gl_program_map_) {
            if (program.second != 0) {
                delete program.second;
                program.second = 0;
            }
        }
        gl_program_map_.clear();

        /// gl_geometry
        for (auto &geometry : gl_geometry_map_) {
            if (geometry.second != 0) {
                delete geometry.second;
                geometry.second = 0;
            }
        }
        gl_geometry_map_.clear();

        /// pbr resources
        pbr_resources_.reset();

        /// gl context
        ksGpuWindow_Destroy(&window_);
    }

    uint32_t OpenGLESGraphicsPlugin::GetDepthTexture(uint32_t colorTexture) {
        // If a depth-stencil view has already been created for this back-buffer, use it.
        auto depthBufferIt = color_to_depth_map_.find(colorTexture);
        if (depthBufferIt != color_to_depth_map_.end()) {
            return depthBufferIt->second;
        }

        // This back-buffer has no corresponding depth-stencil texture, so create one with matching dimensions.

        GLint width;
        GLint height;
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

        uint32_t depthTexture;
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT,
                     nullptr);

        color_to_depth_map_.insert(std::make_pair(colorTexture, depthTexture));

        return depthTexture;
    }

    void OpenGLESGraphicsPlugin::RenderProjectionView(const XrCompositionLayerProjectionView &layerView,
                                                      const XrSwapchainImageBaseHeader *swapchainImage,
                                                      int64_t swapchainFormat, const std::vector<Scene> &scenes) {
        CHECK(layerView.subImage.imageArrayIndex == 0);  // Texture arrays not supported.
        UNUSED_PARM(swapchainFormat);                    // Not used in this function for now.

        glBindFramebuffer(GL_FRAMEBUFFER, swapchain_framebuffer_);

        const uint32_t colorTexture = reinterpret_cast<const XrSwapchainImageOpenGLESKHR *>(swapchainImage)->image;

        glViewport(static_cast<GLint>(layerView.subImage.imageRect.offset.x),
                   static_cast<GLint>(layerView.subImage.imageRect.offset.y),
                   static_cast<GLsizei>(layerView.subImage.imageRect.extent.width),
                   static_cast<GLsizei>(layerView.subImage.imageRect.extent.height));

        glFrontFace(GL_CW);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        const uint32_t depthTexture = GetDepthTexture(colorTexture);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

        // Clear swapchain and depth buffer.
        glClearColor(clear_color_[0], clear_color_[1], clear_color_[2], clear_color_[3]);
        glClearDepthf(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        const auto &pose = layerView.pose;
        XrMatrix4x4f proj;
        XrMatrix4x4f_CreateProjectionFov(&proj, GRAPHICS_OPENGL_ES, layerView.fov, 0.05f, 100.0f);
        XrMatrix4x4f toView;
        XrVector3f scale{1.f, 1.f, 1.f};
        XrMatrix4x4f_CreateTranslationRotationScale(&toView, &pose.position, &pose.orientation, &scale);
        XrMatrix4x4f view;
        XrMatrix4x4f_InvertRigidBody(&view, &toView);
        XrMatrix4x4f vp;
        XrMatrix4x4f_Multiply(&vp, &proj, &view);

        // Render each scene
        for (const Scene &scene : scenes) {
            // Render each object in the scene
            for (const auto &object : scene.GetAllObjects()) {
                // create geometry
                CreateGLGeometries(*object);

                XrMatrix4x4f model;
                const XrPosef objLoc = object->GetPose();
                const XrVector3f &objScale = object->GetScale();
                XrMatrix4x4f_CreateTranslationRotationScale(&model, &objLoc.position, &objLoc.orientation, &objScale);
                XrMatrix4x4f mvp;
                XrMatrix4x4f_Multiply(&mvp, &vp, &model);
                // log the object location and scale
                PLOGD("object position=(%f, %f, %f), orientation=(%f, %f, %f, %f)", objLoc.position.x,
                      objLoc.position.y, objLoc.position.z, objLoc.orientation.x, objLoc.orientation.y,
                      objLoc.orientation.z, objLoc.orientation.w);
                PLOGD("object scale=(%f, %f, %f)", objScale.x, objScale.y, objScale.z);

                GLProgramType programType = object->GetGLProgramType();
                GLProgram *program = nullptr;
                auto programIt = gl_program_map_.find(programType);
                if (programIt != gl_program_map_.end()) {
                    program = programIt->second;
                } else if (programType != GL_PROGRAM_TYPE_PBR_SELF_OWNED) {
                    PLOGE("render error: unknown programType=%d", programType);
                    continue;
                } else {
                    // use default program
                    program = gl_program_map_[GL_PROGRAM_TYPE_COLOR];
                }

                GLGeometryType geometryType = object->GetGLGeometryType();
                auto geometryIt = gl_geometry_map_.find(geometryType);
                GLGeometry *geometry = nullptr;
                if (geometryIt != gl_geometry_map_.end()) {
                    geometry = geometryIt->second;
                } else if (geometryType != GL_GEOMETRY_TYPE_PBR_SELF_OWNED &&
                           geometryType != GL_GEOMETRY_TYPE_POS_COLOR_MESH) {
                    PLOGE("render error: unknown geometryType=%d", geometryType);
                    continue;
                } else {
                    // use default geometry
                    geometry = gl_geometry_map_[GL_GEOMETRY_TYPE_POS_COLOR_MESH];
                }

                // TODO: refactor the geometry management logic
                if (geometryType == GL_GEOMETRY_TYPE_POS_COLOR_MESH) {
                    geometry = new GLGeometry();
                    GLProgramAttribute *vertexAttrib = nullptr;
                    int numAttribs = 0;
                    auto indexBuffer = object->GetDrawOrder();
                    auto vertexBuffer = object->GetVertexBuffer();
                    vertexAttrib = const_cast<GLProgramAttribute *>(pos_color_vert_attrib_);
                    numAttribs = NUM_POS_COLOR_VERT_ATTRIB;
                    DrawMode drawMode = object->GetDrawMode();
                    auto primitiveType = DrawModeToGlPrimitive(drawMode);
                    if (primitiveType == -1) {
                        PLOGE("Unsupported draw mode=%d", drawMode);
                        THROW("Unsupported draw mode");
                    }
                    geometry->Initialize(vertexAttrib, numAttribs, indexBuffer.data(),
                                         indexBuffer.size() * sizeof(uint32_t), vertexBuffer.data(),
                                         vertexBuffer.size() * sizeof(float), primitiveType);
                }

                program->Bind();

                auto enableDepthTest = glIsEnabled(GL_DEPTH_TEST);
                if (object->IsRenderDepthable() != enableDepthTest) {
                    glDepthMask(object->IsRenderDepthable());
                }

                glEnable(GL_BLEND);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_DST_ALPHA);
                glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);

                auto objectType = object->GetType();
                switch (objectType) {
                case OBJECT_TYPE_CUBE: {
                    program->SetUniformMat4("ModelViewProjection", mvp);
                    geometry->Submit();
                    break;
                }
                case OBJECT_TYPE_TRUNCATED_CONE: {
                    glDisable(GL_CULL_FACE);
                    program->SetUniformMat4("ModelViewProjection", mvp);
                    geometry->Submit();
                    glEnable(GL_CULL_FACE);
                    break;
                }
                case OBJECT_TYPE_CARTESIAN_BRANCH: {
                    glLineWidth(3.0f);
                    program->SetUniformMat4("ModelViewProjection", mvp);
                    geometry->Submit();
                    break;
                }
                case OBJECT_TYPE_GUI_PLANE: {
                    uint32_t colorTexId = object->GetColorTexId();
                    program->SetUniformMat4("ModelViewProjection", mvp);
                    program->SetUniformSampler("u_texture", colorTexId, GL_TEXTURE_2D);
                    geometry->Submit();
                    break;
                }
                case OBJECT_TYPE_SKYBOX: {
                    // disable cull face
                    glDisable(GL_CULL_FACE);
                    glDepthFunc(GL_LEQUAL);
                    uint32_t skyboxTexId = object->GetColorTexId();
                    // use view matrix without translation
                    XrMatrix4x4f viewWithoutTranslationInv;
                    XrVector3f origin{0.0f, 0.0f, 0.0f};
                    XrMatrix4x4f_CreateTranslationRotationScale(&viewWithoutTranslationInv, &origin, &pose.orientation,
                                                                &scale);
                    XrMatrix4x4f viewWithoutTranslation;
                    XrMatrix4x4f_InvertRigidBody(&viewWithoutTranslation, &viewWithoutTranslationInv);
                    XrMatrix4x4f viewProjection;
                    XrMatrix4x4f_Multiply(&viewProjection, &proj, &viewWithoutTranslation);

                    program->SetUniformMat4("ModelViewProjection", viewProjection);
                    program->SetUniformSampler("u_texture", skyboxTexId, GL_TEXTURE_CUBE_MAP);
                    geometry->Submit();
                    // Reset depth function.
                    glDepthFunc(GL_LESS);
                    // enable cull face
                    glEnable(GL_CULL_FACE);
                    break;
                }
                case OBJECT_TYPE_MESH: {
                    // double side visible as default
                    glDisable(GL_CULL_FACE);
                    program->SetUniformMat4("ModelViewProjection", mvp);
                    geometry->Submit();
                    glEnable(GL_CULL_FACE);
                    break;
                }
                case OBJECT_TYPE_GLTF_MODEL: {
                    // specially for gltf model, use self owned render func
                    auto gltfObj = std::dynamic_pointer_cast<GltfModel>(object);
                    if (nullptr != gltfObj && gltfObj->IsValid()) {
                        // clear gl error, TODO: check what is the error
                        auto error = glGetError();
                        if (error != GL_NO_ERROR) {
                            PLOGD("before render gltf model occur render error=%d at object id: %lld type: %d", error,
                                  object->GetId(), objectType);
                        }
                        gltfObj->Render(pbr_resources_.get(), view, proj);
                    }
                    break;
                }
                default: {
                    PLOGW("Unimplemented object type=%d", objectType);
                    break;
                }
                }

                auto error = glGetError();
                if (error != GL_NO_ERROR) {
                    PLOGD("clear gl error=%d at object id: %lld type: %d after render", error, object->GetId(),
                          objectType);
                }

                // check if wireframe is enabled
                auto wireframeIt = gl_program_map_.find(GL_PROGRAM_TYPE_COLOR_WIREFRAME);
                if (wireframeIt != gl_program_map_.end()) {
                    GLProgram *wireframeProgram = wireframeIt->second;
                    if (object->IsWireframeEnabled() && object->GetDrawMode() == DRAW_MODE_TRIANGLES) {
                        wireframeProgram->Bind();
                        glLineWidth(1.0f);
                        wireframeProgram->SetUniformVec3("wireframeColor", object->GetWireframeColor());
                        wireframeProgram->SetUniformMat4("ModelViewProjection", mvp);
                        geometry->Submit();
                        wireframeProgram->UnBind();
                    }
                } else {
                    PLOGW("wireframe program not found");
                }

                // TODO: refactor the geometry management logic
                if (geometryType == GL_GEOMETRY_TYPE_POS_COLOR_MESH) {
                    delete geometry;
                    geometry = nullptr;
                }

                // reset
                glDisable(GL_BLEND);
                glDepthMask(enableDepthTest);

                program->UnBind();
            }
        }

        glBindVertexArray(0);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    int64_t OpenGLESGraphicsPlugin::SelectColorSwapchainFormat(const std::vector<int64_t> &runtimeFormats) const {
        // List of supported color swapchain formats.
        std::vector<int64_t> supportedColorSwapchainFormats{GL_RGBA8, GL_RGBA8_SNORM};

        // In OpenGLES 3.0+, the R, G, and B values after blending are converted into the non-linear
        // sRGB automatically.
        if (context_api_major_version_ >= 3) {
            supportedColorSwapchainFormats.push_back(GL_SRGB8_ALPHA8);
        }

        auto swapchainFormatIt =
                std::find_first_of(runtimeFormats.begin(), runtimeFormats.end(), supportedColorSwapchainFormats.begin(),
                                   supportedColorSwapchainFormats.end());
        if (swapchainFormatIt == runtimeFormats.end()) {
            THROW("No runtime swapchain format supported for color swapchain");
        }

        return *swapchainFormatIt;
    }

    int64_t OpenGLESGraphicsPlugin::SelectDepthSwapchainFormat(const std::vector<int64_t> &runtimeFormats) const {
        // List of supported depth swapchain formats.
        const std::array<GLenum, 4> f{
                GL_DEPTH24_STENCIL8,
                GL_DEPTH_COMPONENT24,
                GL_DEPTH_COMPONENT16,
                GL_DEPTH_COMPONENT32F,
        };

        auto swapchainFormatIt = std::find_first_of(runtimeFormats.begin(), runtimeFormats.end(), f.begin(), f.end());
        if (swapchainFormatIt == runtimeFormats.end()) {
            THROW("No runtime swapchain format supported for depth swapchain");
        }

        return *swapchainFormatIt;
    }

    int64_t
    OpenGLESGraphicsPlugin::SelectMotionVectorSwapchainFormat(const std::vector<int64_t> &runtimeFormats) const {
        // List of swapchain formats suitable for motion vectors.
        const std::array<GLenum, 1> f{
                GL_RGBA16F,
        };

        auto swapchainFormatIt = std::find_first_of(runtimeFormats.begin(), runtimeFormats.end(), f.begin(), f.end());
        if (swapchainFormatIt == runtimeFormats.end()) {
            THROW("No runtime swapchain format supported for motion vector swapchain");
        }
        return *swapchainFormatIt;
    }

    std::vector<XrSwapchainImageBaseHeader *> OpenGLESGraphicsPlugin::AllocateSwapchainImageStructs(
            uint32_t capacity, const XrSwapchainCreateInfo &swapchainCreateInfo) {
        // Allocate and Initialize the buffer of image structs (must be sequential in memory for
        // xrEnumerateSwapchainImages).
        // Return back an array of pointers to each swapchain image struct so the consumer
        // doesn't need to know the type/size.
        std::vector<XrSwapchainImageOpenGLESKHR> swapchainImageBuffer(capacity,
                                                                      {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
        std::vector<XrSwapchainImageBaseHeader *> swapchainImageBase;
        for (XrSwapchainImageOpenGLESKHR &image : swapchainImageBuffer) {
            swapchainImageBase.push_back(reinterpret_cast<XrSwapchainImageBaseHeader *>(&image));
        }

        // Keep the buffer alive by moving it into the list of buffers.
        swapchain_image_buffers_.push_back(std::move(swapchainImageBuffer));

        return swapchainImageBase;
    }

    const XrBaseInStructure *OpenGLESGraphicsPlugin::GetGraphicsBinding() const {
        return reinterpret_cast<const XrBaseInStructure *>(&graphics_binding_);
    }

    void OpenGLESGraphicsPlugin::UpdateConfigurationsAtGraphics(const std::shared_ptr<struct Configurations> &config) {
        clear_color_ = config->GetBackgroundClearColor();
    }

    void OpenGLESGraphicsPlugin::CreateGLGeometries(const Object &object) {
        GLGeometryType geometryType = object.GetGLGeometryType();
        auto geometryIt = gl_geometry_map_.find(geometryType);
        if (geometryIt != gl_geometry_map_.end()) {
            PLOGD("Geometry already exists");
            return;
        }

        GLGeometry *geometry = new GLGeometry();
        GLProgramAttribute *vertexAttrib = nullptr;
        int numAttribs = 0;
        auto indexBuffer = object.GetDrawOrder();
        auto vertexBuffer = object.GetVertexBuffer();
        switch (geometryType) {
        case GL_GEOMETRY_TYPE_POS_COLOR_CUBE: {
            vertexAttrib = const_cast<GLProgramAttribute *>(pos_color_vert_attrib_);
            numAttribs = NUM_POS_COLOR_VERT_ATTRIB;
            break;
        }
        case GL_GEOMETRY_TYPE_POS_COLOR_TRUNCATED_CONE: {
            vertexAttrib = const_cast<GLProgramAttribute *>(pos_color_vert_attrib_);
            numAttribs = NUM_POS_COLOR_VERT_ATTRIB;
            break;
        }
        case GL_GEOMETRY_TYPE_POS_COLOR_CARTESIAN_BRANCH: {
            vertexAttrib = const_cast<GLProgramAttribute *>(pos_color_vert_attrib_);
            numAttribs = NUM_POS_COLOR_VERT_ATTRIB;
            break;
        }
        case GL_GEOMETRY_TYPE_POS_SAMPLER2D_QUAD: {
            vertexAttrib = const_cast<GLProgramAttribute *>(pos_color_vert_attrib_);
            numAttribs = NUM_POS_COLOR_VERT_ATTRIB;
            break;
        }
        case GL_GEOMETRY_TYPE_POS_SAMPLERCUBE_SKYBOX: {
            vertexAttrib = const_cast<GLProgramAttribute *>(pos_vert_attrib_);
            numAttribs = NUM_POS_VERT_ATTRIB;
            break;
        }
        default:
            PLOGE("Unsupported geometry type=%d", geometryType);
            delete geometry;
            return;
        }

        DrawMode drawMode = object.GetDrawMode();
        auto primitiveType = DrawModeToGlPrimitive(drawMode);
        if (primitiveType == -1) {
            PLOGE("Unsupported draw mode=%d", drawMode);
            THROW("Unsupported draw mode");
        }

        geometry->Initialize(vertexAttrib, numAttribs, indexBuffer.data(), indexBuffer.size() * sizeof(uint32_t),
                             vertexBuffer.data(), vertexBuffer.size() * sizeof(float), primitiveType);
        gl_geometry_map_[geometryType] = geometry;
    }

    void OpenGLESGraphicsPlugin::SetBackgroundColor(std::array<float, 4> color) {
        clear_color_ = color;
    }

    void OpenGLESGraphicsPlugin::ClearSwapchainCache() {
        swapchain_image_data_map_.Reset();
    }

    void OpenGLESGraphicsPlugin::Flush() {
        GL(glFlush());
    }

    void OpenGLESGraphicsPlugin::CopyRGBAImage(const XrSwapchainImageBaseHeader *swapchainImage, uint32_t arraySlice,
                                               const Conformance::RGBAImage &image) {
        OpenGLESSwapchainImageData *swapchainData;
        uint32_t imageIndex;

        std::tie(swapchainData, imageIndex) = swapchain_image_data_map_.GetDataAndIndexFromBasePointer(swapchainImage);

        const bool isArray = swapchainData->HasMultipleSlices();
        GLuint width = swapchainData->Width();
        GLuint height = swapchainData->Height();
        GLenum target = isArray ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

        const uint32_t img = swapchainData->GetTypedImage(imageIndex).image;
        GL(glBindTexture(target, img));
        if (isArray) {
            for (GLuint y = 0; y < height; ++y) {
                const void *pixels = &image.pixels[(height - 1 - y) * width];
                GL(glTexSubImage3D(target, 0, 0, y, arraySlice, width, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels));
            }
        } else {
            for (GLuint y = 0; y < height; ++y) {
                const void *pixels = &image.pixels[(height - 1 - y) * width];
                GL(glTexSubImage2D(target, 0, 0, y, width, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels));
            }
        }
        GL(glBindTexture(target, 0));
    }

    int64_t OpenGLESGraphicsPlugin::GetSRGBA8Format() const {
        return GL_SRGB8_ALPHA8;
    }

    ISwapchainImageData *
    OpenGLESGraphicsPlugin::AllocateSwapchainImageData(size_t size, const XrSwapchainCreateInfo &swapchainCreateInfo) {
        auto typedResult =
                std::make_unique<OpenGLESSwapchainImageData>(static_cast<uint32_t>(size), swapchainCreateInfo);

        // Cast our derived type to the caller-expected type.
        auto ret = static_cast<ISwapchainImageData *>(typedResult.get());

        swapchain_image_data_map_.Adopt(std::move(typedResult));

        return ret;
    }

    ISwapchainImageData *OpenGLESGraphicsPlugin::AllocateSwapchainImageDataWithDepthSwapchain(
            size_t size, const XrSwapchainCreateInfo &colorSwapchainCreateInfo, XrSwapchain depthSwapchain,
            const XrSwapchainCreateInfo &depthSwapchainCreateInfo) {
        auto typedResult = std::make_unique<OpenGLESSwapchainImageData>(
                static_cast<uint32_t>(size), colorSwapchainCreateInfo, depthSwapchain, depthSwapchainCreateInfo);

        // Cast our derived type to the caller-expected type.
        auto ret = static_cast<ISwapchainImageData *>(typedResult.get());

        swapchain_image_data_map_.Adopt(std::move(typedResult));

        return ret;
    }

    void OpenGLESGraphicsPlugin::ClearImageSlice(const XrSwapchainImageBaseHeader *colorSwapchainImage,
                                                 uint32_t imageArrayIndex, XrColor4f color) {
        OpenGLESSwapchainImageData *swapchainData;
        uint32_t imageIndex;

        std::tie(swapchainData, imageIndex) =
                swapchain_image_data_map_.GetDataAndIndexFromBasePointer(colorSwapchainImage);

        const bool isArray = swapchainData->HasMultipleSlices();
        GLenum target = isArray ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

        GL(glBindFramebuffer(GL_FRAMEBUFFER, swapchain_framebuffer_));

        const uint32_t colorTexture = swapchainData->GetTypedImage(imageIndex).image;
        const uint32_t depthTexture = swapchainData->GetDepthImageForColorIndex(imageIndex).image;
        if (isArray) {
            GL(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorTexture, 0, imageArrayIndex));
            GL(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0, imageArrayIndex));
        } else {
            GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, colorTexture, 0));
            GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, depthTexture, 0));
        }

        GLint x = 0;
        GLint y = 0;
        GLsizei w = swapchainData->Width();
        GLsizei h = swapchainData->Height();
        GL(glViewport(x, y, w, h));
        GL(glScissor(x, y, w, h));

        GL(glEnable(GL_SCISSOR_TEST));

        // Clear swapchain and depth buffer.
        GL(glClearColor(color.r, color.g, color.b, color.a));
        GL(glClearDepthf(1.0f));
        GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLESGraphicsPlugin::ClearImageSlice(const XrSwapchainImageBaseHeader *colorSwapchainImage,
                                                 uint32_t imageArrayIndex) {
        XrColor4f clearColor = {
                clear_color_[0],
                clear_color_[1],
                clear_color_[2],
                clear_color_[3],
        };
        ClearImageSlice(colorSwapchainImage, imageArrayIndex, clearColor);
    }

    Pbr::IGltfBuilder *OpenGLESGraphicsPlugin::GetPbrResources() {
        return pbr_resources_.get();
    }

    GLGeometry *OpenGLESGraphicsPlugin::GetGLGeometry(uint32_t type) {
        if (type < GL_PROGRAM_TYPE_COLOR || type >= GL_PROGRAM_TYPE_NUM || type == GL_PROGRAM_TYPE_PBR_SELF_OWNED) {
            PLOGW("GetGLGeometry: unknown type %d", type);
            return nullptr;
        }
        auto it = gl_geometry_map_.find(type);
        if (it != gl_geometry_map_.end()) {
            return it->second;
        }
        return nullptr;
    }
}  // namespace PVRSampleFW
