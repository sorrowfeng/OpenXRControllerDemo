// Copyright (c) 2022-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: MIT

/*
 * Copyright 2024 - 2025 PICO.
 *
 * This code is a PICO modified version from OpenXR SDK distribution by The Khronos Group Inc.
 *
 * */

#ifndef PICONATIVEOPENXRSAMPLES_GLTFUTILS_H
#define PICONATIVEOPENXRSAMPLES_GLTFUTILS_H

#include "gltf/GltfHelper.h"
#include "pbr/GltfLoader.h"
#include "PbrSharedState.h"
#include <memory>

namespace tinygltf {
    class Model;
    class TinyGLTF;
}  // namespace tinygltf

namespace PVRSampleFW {
    /// Templated base class for API-specific model objects in the main CTS code.
    template <typename ModelInstanceType, typename ResourcesType>
    class RenderableGltfModelInstanceBase {
    public:
        explicit RenderableGltfModelInstanceBase(ModelInstanceType&& pbrModelInstance,
                                                 Pbr::FillMode fillMode = Pbr::FillMode::Solid)
            : pbr_model_instance_(std::move(pbrModelInstance)), fill_model_(fillMode) {
        }

        ModelInstanceType& GetModelInstance() noexcept {
            return pbr_model_instance_;
        }
        const ModelInstanceType& GetModelInstance() const noexcept {
            return pbr_model_instance_;
        }

        void SetFillMode(const Pbr::FillMode& fillMode) {
            fill_model_ = fillMode;
        }

        Pbr::FillMode GetFillMode() const noexcept {
            return fill_model_;
        }

        void SetBaseColorFactor(const ResourcesType& pbrResources, Pbr::RGBAColor color) {
            for (uint32_t k = 0; k < pbr_model_instance_.GetPrimitiveCount(); k++) {
                auto& material = pbrResources.GetPrimitive(pbr_model_instance_.GetPrimitiveHandle(k)).GetMaterial();
                material->Parameters().BaseColorFactor = color;
            }
        }

    private:
        std::shared_ptr<const tinygltf::Model> gltf_;
        ModelInstanceType pbr_model_instance_;
        Pbr::FillMode fill_model_;
    };
}  // namespace PVRSampleFW

/// Load a glTF file from memory into a shared pointer, throwing on errors.
std::shared_ptr<const tinygltf::Model> LoadGLTF(nonstd::span<const uint8_t> data);

/// Load a glTF file from memory into a shared pointer, throwing on errors, using the provided loader.
std::shared_ptr<const tinygltf::Model> LoadGLTF(nonstd::span<const uint8_t> data, tinygltf::TinyGLTF* loader);

#endif  //PICONATIVEOPENXRSAMPLES_GLTFUTILS_H
