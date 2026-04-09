// Copyright (c) 2022-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: MIT

/*
 * Copyright 2024 - 2025 PICO.
 *
 * This code is a PICO modified version from OpenXR SDK distribution by The Khronos Group Inc.
 *
 * */

#include "GltfUtils.h"
#include <nonstd/span.hpp>
#include "gltf/GltfHelper.h"
#include "LogUtils.h"
#include "CheckUtils.h"
#include <tiny_gltf.h>

#define TINYGLTF_IMPLEMENTATION

#if defined(_MSC_VER)
#pragma warning(disable : 4018)  // signed/unsigned mismatch
#pragma warning(disable : 4189)  // local variable is initialized but not referenced
#endif                           // defined(_MSC_VER)

std::shared_ptr<const tinygltf::Model> LoadGLTF(nonstd::span<const uint8_t> data, tinygltf::TinyGLTF* loader) {
    std::shared_ptr<tinygltf::Model> model = std::make_shared<tinygltf::Model>();
    std::string err;
    std::string warn;
    loader->SetImageLoader(GltfHelper::PassThroughKTX2, nullptr);
    bool loadedModel = loader->LoadBinaryFromMemory(model.get(), &err, &warn, data.data(), (unsigned int)data.size());
    if (!warn.empty()) {
        PLOGW("glTF WARN: %s", warn.c_str());
    }

    if (!err.empty()) {
        THROW("glTF ERR: " + err);
    }

    if (!loadedModel) {
        THROW("Failed to load glTF model provided.");
    }
    return std::const_pointer_cast<const tinygltf::Model>(std::move(model));
}

std::shared_ptr<const tinygltf::Model> LoadGLTF(nonstd::span<const uint8_t> data) {
    tinygltf::TinyGLTF loader;

    return LoadGLTF(data, &loader);
}
