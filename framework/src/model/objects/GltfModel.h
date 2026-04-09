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

#ifndef PICONATIVEOPENXRSAMPLES_GLTFMODEL_H
#define PICONATIVEOPENXRSAMPLES_GLTFMODEL_H

#include "Object.h"
#include "nonstd/type.hpp"
#include "GltfLoader.h"
#include "Common.h"
#include "GLGltfHelper.h"
#include <atomic>
#include <future>
#include "PbrModelUtils.h"

namespace PVRSampleFW {
    // Policy to default-init to max so we can tell that a "null" handle is bad.
    // Otherwise, a default-init would be 0 which is often a perfectly fine index.
    using custom_default_max_uint64 = nonstd::custom_default_t<uint64_t, std::numeric_limits<uint64_t>::max()>;

    /// Handle returned by a graphics plugin, used to reference plugin-internal data for a loaded glTF (PBR) model.
    ///
    /// They expire at IGraphicsPlugin::Shutdown() and IGraphicsPlugin::ShutdownDevice() calls,
    /// so must not be persisted past those calls.
    ///
    /// They are "null" by default, so may be tested for validity by comparison against a default-constructed instance.
    using GLTFModelHandle = nonstd::equality<uint64_t, struct GLTFModelHandleTag, custom_default_max_uint64>;

    /// Handle returned by a graphics plugin, used to reference plugin-internal data for a renderable
    /// instance of a glTF (PBR) model.
    ///
    /// They expire at IGraphicsPlugin::Shutdown() and IGraphicsPlugin::ShutdownDevice() calls,
    /// so must not be persisted past those calls.
    ///
    /// They are "null" by default, so may be tested for validity by comparison against a default-constructed instance.
    using GLTFModelInstanceHandle =
            nonstd::equality<uint64_t, struct GLTFModelInstanceHandleTag, custom_default_max_uint64>;

    class GltfModel : public Object {
    public:
        GltfModel();
        GltfModel(const XrPosef& p, const XrVector3f& s);
        GltfModel(const XrPosef& p, const XrVector3f& s, std::function<Gltf::ModelBuilder()> func,
                  Pbr::IGltfBuilder* gltfBuilder, bool async);
        ~GltfModel();

        /**
         * Set ModelBuilder function and build type
         * @param func model builder function, to load and parse gltf file
         * @param async whether async build
         */
        void SetBuilderFunction(std::function<Gltf::ModelBuilder()> func, bool async);

        /**
         * Build a gltf model object. Build type depends on async parameter from SetBuilderFunction
         *
         * @note: If you use async build type, You have to constantly check the validity of
         * the object and call the function that returns it to you when it is not yet valid.
         *
         * @param gltfBuilder pbr resources, you can get it from graphic plugin
         * @return nullptr if you use sync build method, a function to check and load model to gpu in future
         * if you use async build method
         */
        std::function<void()> BuildObject(Pbr::IGltfBuilder* gltfBuilder);

        /**
         * Check validity of this object. You can add it to Scene when valid
         * @return true or false
         */
        bool IsValid() const {
            return is_valid_;
        }

        /**
         * Self render func in special
         * @param gltfBuilder pbr resources, you can get it from graphic plugin
         * @param view view matrix
         * @param projection projection matrix
         */
        void Render(Pbr::IGltfBuilder* gltfBuilder, XrMatrix4x4f view, XrMatrix4x4f projection);

        // TODO: collision not ready

    public:
        /// Get the underlying Pbr::Model associated with the supplied handle.
        std::shared_ptr<Pbr::Model> GetPbrModel(GLTFModelHandle handle);

        /// Get a reference to the base interface for a given ModelInstance from its handle
        Pbr::ModelInstance& GetModelInstance(GLTFModelInstanceHandle handle);

        GLTFModelHandle GetDefaultGLTFModelHandle() const {
            return default_gltf_model_handle_;
        }

        GLTFModelInstanceHandle GetDefaultGLTFModelInstanceHandle() const {
            return default_gltf_model_instance_handle_;
        }

        void SetPbrModelAnimationHandler(const std::shared_ptr<PbrModelAnimationHandler>& handler) {
            pbr_model_animation_handler_ = handler;
        }

        int UpdateAnimation(std::vector<PbrNodeState>&& nodeStates);

        std::shared_ptr<PbrModelAnimationHandler> GetPbrModelAnimationHandler() {
            return pbr_model_animation_handler_;
        }

    private:
        /// Create internal data for a glTF model, returning a handle to refer to it.
        /// This handle expires when the internal data is cleared in ~GltfModel().
        GLTFModelHandle LoadGLTF(Gltf::ModelBuilder&& modelBuilder, Pbr::IGltfBuilder* gltfBuilder);

        /// Create a renderable instance of a glTF model, returning a handle to refer to it.
        /// This handle expires when the internal data is cleared in ~GltfModel().
        GLTFModelInstanceHandle CreateGLTFModelInstance(GLTFModelHandle handle, Pbr::IGltfBuilder* gltfBuilder);

    private:
        VectorWithGenerationCountedHandles<std::shared_ptr<Pbr::Model>, GLTFModelHandle> gltf_models_;
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
        VectorWithGenerationCountedHandles<GLGLTF, GLTFModelInstanceHandle> gltf_instances_;
#elif defined(XR_USE_GRAPHICS_API_VULKAN)
        VectorWithGenerationCountedHandles<VulkanGLTF, GLTFModelInstanceHandle> gltf_instances_;
#endif
        std::vector<GLTFModelHandle> gltf_model_handles_;
        GLTFModelHandle default_gltf_model_handle_;
        std::vector<GLTFModelInstanceHandle> gltf_model_instance_handles_;
        GLTFModelInstanceHandle default_gltf_model_instance_handle_;
        std::shared_ptr<PbrModelAnimationHandler> pbr_model_animation_handler_{nullptr};

        std::atomic_bool is_valid_{false};
        std::atomic_bool is_build_started_{false};
        std::atomic_bool is_async_build_{false};
        std::function<Gltf::ModelBuilder()> make_gltf_builder_func_{nullptr};
        std::future<Gltf::ModelBuilder> async_make_gltf_builder_task_;
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_GLTFMODEL_H
