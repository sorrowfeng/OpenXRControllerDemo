// Copyright (c) 2017-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

/*
 * Copyright 2024 - 2025 PICO.
 *
 * This code is a PICO modified version from OpenXR SDK distribution by The Khronos Group Inc.
 *
 * */

#ifndef PICONATIVEOPENXRSAMPLES_GLGLTFHELPER_H
#define PICONATIVEOPENXRSAMPLES_GLGLTFHELPER_H

#include "OpenGL/GLModel.h"
#include "OpenGL/GLResources.h"
#include "GltfUtils.h"

namespace Pbr {
    class GLModel;
    struct GLResources;
}  // namespace Pbr

namespace PVRSampleFW {
    class GLGLTF : public RenderableGltfModelInstanceBase<Pbr::GLModelInstance, Pbr::GLResources> {
    public:
        using RenderableGltfModelInstanceBase::RenderableGltfModelInstanceBase;

        void Render(Pbr::GLResources* resources, const XrMatrix4x4f& modelToWorld) {
            resources->SetFillMode(GetFillMode());
            resources->Bind();
            GetModelInstance().Render(*resources, modelToWorld);
        }
    };
}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_GLGLTFHELPER_H
