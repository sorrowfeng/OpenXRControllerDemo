/*
 * Copyright 2025 - 2025 PICO. All rights reserved.  
 *
 * NOTICE: All information contained herein is, and remains the property of PICO.
 * The intellectual and technical concepts contained herein are proprietary to PICO.
 * and may be covered by patents, patents in process, and are protected by trade
 * secret or copyright law. Dissemination of this information or reproduction of
 * this material is strictly forbidden unless prior written permission is obtained
 * from PICO.
 */

#include "GltfModel.h"
#include "xr_math_operators.h"

namespace PVRSampleFW {
    GltfModel::GltfModel() {
        type_ = OBJECT_TYPE_GLTF_MODEL;
        gl_program_type_ = GL_PROGRAM_TYPE_PBR_SELF_OWNED;
        gl_geometry_type_ = GL_GEOMETRY_TYPE_PBR_SELF_OWNED;
    }

    GltfModel::GltfModel(const XrPosef &p, const XrVector3f &s) {
        type_ = OBJECT_TYPE_GLTF_MODEL;
        gl_program_type_ = GL_PROGRAM_TYPE_PBR_SELF_OWNED;
        gl_geometry_type_ = GL_GEOMETRY_TYPE_PBR_SELF_OWNED;
        pose_ = p;
        scale_ = s;
    }

    GltfModel::~GltfModel() {
        // clear gltf models and instances
        gltf_instances_.clear();
        gltf_model_instance_handles_.clear();
        gltf_model_handles_.clear();
        gltf_models_.clear();
    }

    GLTFModelHandle GltfModel::LoadGLTF(Gltf::ModelBuilder &&modelBuilder, Pbr::IGltfBuilder *gltfBuilder) {
        auto handle = gltf_models_.emplace_back(modelBuilder.Build(*gltfBuilder));
        gltf_model_handles_.push_back(handle);
        return handle;
    }

    std::shared_ptr<Pbr::Model> GltfModel::GetPbrModel(GLTFModelHandle handle) {
        return gltf_models_[handle];
    }

    GLTFModelInstanceHandle GltfModel::CreateGLTFModelInstance(GLTFModelHandle handle, Pbr::IGltfBuilder *gltfBuilder) {
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
        auto pbrResource = dynamic_cast<Pbr::GLResources *>(gltfBuilder);
#elif defined(XR_USE_GRAPHICS_API_VULKAN)
        // TODO: implement vulkan render
#endif
        auto pbrModelInstance = Pbr::GLModelInstance(*pbrResource, GetPbrModel(handle));
        auto instanceHandle = gltf_instances_.emplace_back(std::move(pbrModelInstance));
        gltf_model_instance_handles_.push_back(instanceHandle);
        return instanceHandle;
    }

    Pbr::ModelInstance &GltfModel::GetModelInstance(GLTFModelInstanceHandle handle) {
        return gltf_instances_[handle].GetModelInstance();
    }

    void GltfModel::Render(Pbr::IGltfBuilder *gltfBuilder, XrMatrix4x4f view, XrMatrix4x4f projection) {
        for (auto &instance : gltf_model_instance_handles_) {
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
            GLGLTF &gltf = gltf_instances_[instance];
            XrMatrix4x4f modelToWorld =
                    Pbr::Matrix::FromTranslationRotationScale(pose_.position, pose_.orientation, scale_);
            auto pbrResource = dynamic_cast<Pbr::GLResources *>(gltfBuilder);
            pbrResource->SetViewProjection(view, projection);
            gltf.Render(pbrResource, modelToWorld);
#elif defined(XR_USE_GRAPHICS_API_VULKAN)
            // TODO: implement vulkan render
#endif
        }
    }

    void GltfModel::SetBuilderFunction(std::function<Gltf::ModelBuilder()> func, bool async) {
        make_gltf_builder_func_ = func;
        is_async_build_ = async;
    }

    std::function<void()> GltfModel::BuildObject(Pbr::IGltfBuilder *gltfBuilder) {
        /// for async build, setup async future and return 'CheckAndLoadToGpu' function
        if (is_async_build_) {
            if (!is_build_started_ && !async_make_gltf_builder_task_.valid()) {
                async_make_gltf_builder_task_ = std::async(std::launch::async, make_gltf_builder_func_);
                is_build_started_ = true;
            }
            std::function<void()> checkAndLoadToGpu = [this, gltfBuilder]() {
                if (async_make_gltf_builder_task_.valid() &&
                    async_make_gltf_builder_task_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    auto builder = async_make_gltf_builder_task_.get();
                    default_gltf_model_handle_ = LoadGLTF(std::move(builder), gltfBuilder);
                    default_gltf_model_instance_handle_ =
                            CreateGLTFModelInstance(default_gltf_model_handle_, gltfBuilder);
                    is_valid_ = true;
                    is_build_started_ = false;
                }
            };
            return checkAndLoadToGpu;
        } else {
            /// for sync build, call builder function directly
            auto builder = make_gltf_builder_func_();
            default_gltf_model_handle_ = LoadGLTF(std::move(builder), gltfBuilder);
            default_gltf_model_instance_handle_ = CreateGLTFModelInstance(default_gltf_model_handle_, gltfBuilder);
            is_valid_ = true;
            return nullptr;
        }
    }

    GltfModel::GltfModel(const XrPosef &p, const XrVector3f &s, std::function<Gltf::ModelBuilder()> func,
                         Pbr::IGltfBuilder *gltfBuilder, bool async) {
        GltfModel(p, s);
        SetBuilderFunction(func, async);
    }

    int GltfModel::UpdateAnimation(std::vector<PbrNodeState> &&nodeStates) {
        if (nullptr == pbr_model_animation_handler_) {
            PLOGW("UpdateAnimation failed for pbr_model_animation_handler_ is nullptr");
            return -1;
        }
        if (!IsValid()) {
            PLOGW("UpdateAnimation failed for gltf model is not valid");
            return -1;
        }
        auto &pbrModelInstance = GetModelInstance(GetDefaultGLTFModelInstanceHandle());
        pbr_model_animation_handler_->UpdateNodes(std::move(nodeStates), &pbrModelInstance);
        return 0;
    }
}  // namespace PVRSampleFW
