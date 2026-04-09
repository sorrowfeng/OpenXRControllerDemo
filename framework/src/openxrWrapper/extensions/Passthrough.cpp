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

#include "Passthrough.h"
#include "BasicOpenXrWrapper.h"

namespace PVRSampleFW {
    std::vector<std::string> Passthrough::GetRequiredExtensions() const {
        return {
                XR_FB_PASSTHROUGH_EXTENSION_NAME,
                XR_FB_TRIANGLE_MESH_EXTENSION_NAME,
        };
    }

    bool Passthrough::OnInstanceCreate() {
        // Initialize the function pointers
        if (openxr_wrapper_ == nullptr) {
            PLOGE("Passthrough::OnInstanceCreate failed for openxr_wrapper_ is null");
            return false;
        }

        auto is_passthrough_enabled = openxr_wrapper_->IsExtensionEnabled(XR_FB_PASSTHROUGH_EXTENSION_NAME);
        auto is_triangle_mesh_enabled = openxr_wrapper_->IsExtensionEnabled(XR_FB_TRIANGLE_MESH_EXTENSION_NAME);

        if (is_passthrough_enabled) {
            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrCreatePassthroughFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrCreatePassthroughFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrDestroyPassthroughFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrDestroyPassthroughFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrPassthroughStartFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrPassthroughStartFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrPassthroughPauseFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrPassthroughPauseFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrCreatePassthroughLayerFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrCreatePassthroughLayerFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrDestroyPassthroughLayerFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrDestroyPassthroughLayerFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrPassthroughLayerPauseFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrPassthroughLayerPauseFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrPassthroughLayerResumeFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrPassthroughLayerResumeFB)));

            // TODO: are these needed?
            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrPassthroughLayerSetStyleFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrPassthroughLayerSetStyleFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrCreateGeometryInstanceFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrCreateGeometryInstanceFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrDestroyGeometryInstanceFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrDestroyGeometryInstanceFB)));

            CHECK_XRCMD(
                    xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrGeometryInstanceSetTransformFB",
                                          reinterpret_cast<PFN_xrVoidFunction *>(&xrGeometryInstanceSetTransformFB)));
        }

        if (is_triangle_mesh_enabled) {
            // TODO: are these needed?
            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrCreateTriangleMeshFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrCreateTriangleMeshFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrDestroyTriangleMeshFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrDestroyTriangleMeshFB)));

            CHECK_XRCMD(
                    xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrTriangleMeshGetVertexBufferFB",
                                          reinterpret_cast<PFN_xrVoidFunction *>(&xrTriangleMeshGetVertexBufferFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrTriangleMeshGetIndexBufferFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrTriangleMeshGetIndexBufferFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrTriangleMeshBeginUpdateFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrTriangleMeshBeginUpdateFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(openxr_wrapper_->GetXrInstance(), "xrTriangleMeshEndUpdateFB",
                                              reinterpret_cast<PFN_xrVoidFunction *>(&xrTriangleMeshEndUpdateFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(
                    openxr_wrapper_->GetXrInstance(), "xrTriangleMeshBeginVertexBufferUpdateFB",
                    reinterpret_cast<PFN_xrVoidFunction *>(&xrTriangleMeshBeginVertexBufferUpdateFB)));

            CHECK_XRCMD(xrGetInstanceProcAddr(
                    openxr_wrapper_->GetXrInstance(), "xrTriangleMeshEndVertexBufferUpdateFB",
                    reinterpret_cast<PFN_xrVoidFunction *>(&xrTriangleMeshEndVertexBufferUpdateFB)));
        }
        return true;
    }

    bool Passthrough::OnSessionCreate() {
        passthrough_support_color_ = passthrough_system_properties_.supportsPassthrough &&
                                     (passthrough_system_properties2_FB_.capabilities &
                                      XR_PASSTHROUGH_CAPABILITY_COLOR_BIT_FB) == XR_PASSTHROUGH_CAPABILITY_COLOR_BIT_FB;
        passthrough_support_depth_ =
                passthrough_system_properties_.supportsPassthrough &&
                (passthrough_system_properties2_FB_.capabilities & XR_PASSTHROUGH_CAPABILITY_LAYER_DEPTH_BIT_FB) ==
                        XR_PASSTHROUGH_CAPABILITY_LAYER_DEPTH_BIT_FB;
        PLOGI(" PassthroughSystemProperties2FB SupportColor= %d SupportDepth= %d\n", passthrough_support_color_,
              passthrough_support_depth_);
        return true;
    }

    bool Passthrough::OnSystemGet(XrSystemProperties *configProperties) {
        passthrough_system_properties_.next = &passthrough_system_properties2_FB_;
        passthrough_system_properties2_FB_.next = configProperties->next;
        configProperties->next = &passthrough_system_properties_;
        return true;
    }

    int Passthrough::Initialize() {
        if (is_initialized_) {
            return XR_SUCCESS;
        }

        if (openxr_wrapper_ != nullptr && openxr_wrapper_->GetXrSession() != XR_NULL_HANDLE &&
            passthrough_system_properties_.supportsPassthrough) {
            XrResult result;
            XrPassthroughCreateInfoFB passthroughCreateInfo = {XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
            passthroughCreateInfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
            CHECK_POINTER_ARG_IS_NOT_NULL(xrCreatePassthroughLayerFB);
            result = xrCreatePassthroughFB(openxr_wrapper_->GetXrSession(), &passthroughCreateInfo, &passthrough_FB_);
            is_initialized_ = result == XR_SUCCESS;
            return result;
        }

        return XR_ERROR_VALIDATION_FAILURE;
    }

    int Passthrough::EnableRegularVideoSeeThrough(bool value) {
        if (value) {
            Initialize();
            CreatePassthroughLayer(999, XR_PASSTHROUGH_LAYER_PURPOSE_PROJECTED_FB);
            EnablePassthroughLayer(999, true);
            Start();
            ResumePassthroughLayer(999);
        } else {
            PausePassthroughLayer(999);
            Pause();
            EnablePassthroughLayer(999, false);
        }
        return XR_SUCCESS;
    }

    bool Passthrough::OnInstanceDestroy() {
        if (passthrough_FB_ != XR_NULL_HANDLE) {
            // destroy geometry instance
            for (auto &geometry_instance : geometry_map_) {
                auto &mesh = geometry_instance.second.mesh_handle;
                if (mesh != XR_NULL_HANDLE) {
                    CHECK_POINTER_ARG_IS_NOT_NULL(xrDestroyTriangleMeshFB);
                    xrDestroyTriangleMeshFB(mesh);
                    mesh = XR_NULL_HANDLE;
                }
                auto &geo = geometry_instance.second.instance_handle;
                if (geo != XR_NULL_HANDLE) {
                    CHECK_POINTER_ARG_IS_NOT_NULL(xrDestroyGeometryInstanceFB);
                    xrDestroyGeometryInstanceFB(geo);
                    geo = XR_NULL_HANDLE;
                }
            }
            geometry_map_.clear();

            // destroy passthrough layer
            for (auto &layer : passthrough_layers_map_) {
                CHECK_POINTER_ARG_IS_NOT_NULL(xrDestroyPassthroughLayerFB);
                xrDestroyPassthroughLayerFB(layer.second);
            }
            passthrough_layers_map_.clear();
            comp_layers_map_.clear();
            enabled_layers_map_.clear();
            CHECK_POINTER_ARG_IS_NOT_NULL(xrDestroyPassthroughFB);
            xrDestroyPassthroughFB(passthrough_FB_);
            passthrough_FB_ = XR_NULL_HANDLE;
            is_initialized_ = false;
        }
        return true;
    }

    bool Passthrough::OnPreEndFrame(std::vector<XrCompositionLayerBaseHeader *> *layers) {
        if (passthrough_system_properties_.supportsPassthrough && is_initialized_) {
            auto size = layers->size();
            for (auto &layer : *layers) {
                auto type = layer->type;
                /// TODO: now all layers except passthrough layer will be set to un-premultiplied alpha
                if (type != XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB) {
                    layer->layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
                    layer->layerFlags |= XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
                    layer->layerFlags |= XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
                }
            }
            // assemble enabled passthrough layers
            std::vector<XrCompositionLayerBaseHeader *> enabled_layers;
            for (auto &layer : comp_layers_map_) {
                auto id = layer.first;
                // check if the layer is enabled
                if (enabled_layers_map_.find(id) != enabled_layers_map_.end() && enabled_layers_map_[id]) {
                    enabled_layers.push_back(
                            reinterpret_cast<XrCompositionLayerBaseHeader *>(&layer.second.passthrough_comp_layer));
                }
            }
            // insert enabled passthrough layers
            layers->insert(layers->begin(), enabled_layers.begin(), enabled_layers.end());
            return true;
        }
        PLOGW("Passthrough::OnPreEndFrame skip for not initialized");
        return true;
    }

    bool Passthrough::OnEventHandlerSetup() {
        /// Here is a demonstration of handling a generic xrEvent.
        if (openxr_wrapper_ == nullptr) {
            PLOGE("Passthrough::OnEventHandlerSetup failed for openxr_wrapper_ is null");
            return false;
        }
        return true;
    }

    bool Passthrough::CreatePassthroughLayer(int id, XrPassthroughLayerPurposeFB purposeType) {
        if (nullptr != openxr_wrapper_ && XR_NULL_HANDLE != openxr_wrapper_->GetXrSession() &&
            passthrough_system_properties_.supportsPassthrough && XR_NULL_HANDLE != passthrough_FB_) {
            if (id < 0) {
                PLOGE("PassthroughLayer id: %d is invalid", id);
                return false;
            }
            // check if layer already created
            if (passthrough_layers_map_.find(id) != passthrough_layers_map_.end()) {
                PLOGI("PassthroughLayer id: %d already created", id);
                return true;
            }
            XrPassthroughLayerFB passthroughLayer = XR_NULL_HANDLE;
            XrResult result;
            XrPassthroughLayerCreateInfoFB passthroughLayerCreateInfo = {XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
            passthroughLayerCreateInfo.passthrough = passthrough_FB_;
            passthroughLayerCreateInfo.purpose = purposeType;
            CHECK_POINTER_ARG_IS_NOT_NULL(xrCreatePassthroughLayerFB);
            result = xrCreatePassthroughLayerFB(openxr_wrapper_->GetXrSession(), &passthroughLayerCreateInfo,
                                                &passthroughLayer);
            if (XR_FAILED(result)) {
                PLOGE("PassthroughLayer failed,Layer no created");
                return false;
            }
            PLOGI("Succeed to create a passthrough layer");
            passthrough_layers_map_[id] = passthroughLayer;

            FBPassthroughCompLayer passthroughCompLayer;
            passthroughCompLayer.id = id;
            passthroughCompLayer.purpose_type = purposeType;
            passthroughCompLayer.passthrough_comp_layer = {XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB};
            passthroughCompLayer.passthrough_comp_layer.layerHandle = passthroughLayer;
            passthroughCompLayer.passthrough_comp_layer.flags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
            passthroughCompLayer.passthrough_comp_layer.space = XR_NULL_HANDLE;
            comp_layers_map_[id] = passthroughCompLayer;
            return true;
        }
        return false;
    }

    bool Passthrough::PausePassthroughLayer(int id) {
        // check if layer exists
        if (passthrough_system_properties_.supportsPassthrough &&
            passthrough_layers_map_.find(id) != passthrough_layers_map_.end()) {
            CHECK_POINTER_ARG_IS_NOT_NULL(xrPassthroughLayerPauseFB);
            XrResult result = xrPassthroughLayerPauseFB(passthrough_layers_map_[id]);
            if (XR_FAILED(result)) {
                PLOGE("xrPassthroughLayerPauseFB failed, ret: %d", result);
                return false;
            }
            return true;
        }

        return false;
    }

    bool Passthrough::ResumePassthroughLayer(int id) {
        // check if layer exists
        if (passthrough_system_properties_.supportsPassthrough &&
            passthrough_layers_map_.find(id) != passthrough_layers_map_.end()) {
            CHECK_POINTER_ARG_IS_NOT_NULL(xrPassthroughLayerResumeFB);
            XrResult result = xrPassthroughLayerResumeFB(passthrough_layers_map_[id]);
            if (XR_FAILED(result)) {
                PLOGE("xrPassthroughLayerResumeFB failed, ret: %d", result);
                return false;
            }
            return true;
        }
        return false;
    }

    bool Passthrough::Pause() {
        if (passthrough_system_properties_.supportsPassthrough && passthrough_FB_ != XR_NULL_HANDLE) {
            // pause all passthrough layers
            for (auto &it : passthrough_layers_map_) {
                CHECK_POINTER_ARG_IS_NOT_NULL(xrPassthroughLayerPauseFB);
                XrResult result = xrPassthroughLayerPauseFB(it.second);
                if (XR_FAILED(result)) {
                    PLOGE("xrPassthroughLayerPauseFB failed, ret: %d", result);
                    return false;
                }
            }

            CHECK_POINTER_ARG_IS_NOT_NULL(xrPassthroughPauseFB);
            XrResult result = xrPassthroughPauseFB(passthrough_FB_);
            if (XR_FAILED(result)) {
                PLOGE("xrPassthroughPauseFB failed, ret: %d", result);
                return false;
            }
            return true;
        }
        return false;
    }

    bool Passthrough::Start() {
        if (passthrough_system_properties_.supportsPassthrough && passthrough_FB_ != XR_NULL_HANDLE) {
            /*// resume all passthrough layers
            for (auto &it : passthrough_layers_map_) {
                CHECK_POINTER_ARG_IS_NOT_NULL(xrPassthroughLayerResumeFB);
                XrResult result = xrPassthroughLayerResumeFB(it.second);
                if (XR_FAILED(result)) {
                    PLOGE("xrPassthroughLayerResumeFB failed, ret: %d", result);
                    return false;
                }
            }*/

            CHECK_POINTER_ARG_IS_NOT_NULL(xrPassthroughStartFB);
            XrResult result = xrPassthroughStartFB(passthrough_FB_);
            if (XR_FAILED(result)) {
                PLOGE("xrPassthroughStartFB failed, ret: %d", result);
                return false;
            }
            return true;
        }
        PLOGE("xrPassthroughStartFB failed, please make sure initialize first!");
        return false;
    }

    int Passthrough::SetPassthroughLayerFBStyle(int id, const XrPassthroughStyleFB &style) {
        if (passthrough_system_properties_.supportsPassthrough &&
            passthrough_layers_map_.find(id) != passthrough_layers_map_.end()) {
            CHECK_POINTER_ARG_IS_NOT_NULL(xrPassthroughLayerSetStyleFB);
            XrResult result = xrPassthroughLayerSetStyleFB(passthrough_layers_map_[id], &style);
            if (XR_FAILED(result)) {
                PLOGE("xrPassthroughLayerSetStyleFB failed, ret: %d", result);
                return XR_ERROR_VALIDATION_FAILURE;
            }
            return XR_SUCCESS;
        }
        PLOGE("SetPassthroughLayerFBStyle failed, please make sure passthrough layer id: %d was created before!", id);
        return XR_ERROR_VALIDATION_FAILURE;
    }

    bool Passthrough::EnablePassthroughLayer(int id, bool value) {
        // check if layer exists
        if (passthrough_system_properties_.supportsPassthrough &&
            passthrough_layers_map_.find(id) != passthrough_layers_map_.end()) {
            enabled_layers_map_[id] = value;
            return true;
        }
        PLOGE("EnablePassthroughLayer failed, please make sure passthrough layer id: %d was created before!", id);
        return false;
    }

    void Passthrough::DestroyPassthroughLayer(int id) {
        // check if layer exists
        if (passthrough_system_properties_.supportsPassthrough &&
            passthrough_layers_map_.find(id) != passthrough_layers_map_.end()) {
            CHECK_POINTER_ARG_IS_NOT_NULL_VOID(xrDestroyPassthroughLayerFB);
            XrResult result = xrDestroyPassthroughLayerFB(passthrough_layers_map_[id]);
            if (XR_FAILED(result)) {
                PLOGE("xrDestroyPassthroughLayerFB failed, ret: %d", result);
            }
            passthrough_layers_map_.erase(id);
            comp_layers_map_.erase(id);
            enabled_layers_map_.erase(id);
            return;
        }

        PLOGI("DestroyPassthroughLayer skip, please make sure passthrough layer id: %d was created before!", id);
    }

    int Passthrough::CreatePassthroughGeometry(const Passthrough::FBPassthroughGeometryCreateInfo &createInfo) {
        auto layer_id = createInfo.layer_id;
        auto geom_id = createInfo.geom_id;
        if (geom_id < 0) {
            PLOGE("PassthroughGeometry id: %d is invalid", geom_id);
            return XR_ERROR_VALIDATION_FAILURE;
        }
        // check geometry exists
        if (geometry_map_.find(geom_id) != geometry_map_.end()) {
            PLOGE("PassthroughGeometry id: %d already created", geom_id);
            return XR_SUCCESS;
        }
        // check if layer exists
        if (passthrough_system_properties_.supportsPassthrough &&
            passthrough_layers_map_.find(layer_id) != passthrough_layers_map_.end()) {
            FBPassthroughGeometry geometry;
            geometry.layer_id = layer_id;
            geometry.id = geom_id;
            geometry.vertices = new XrVector3f[createInfo.vertex_count];
            geometry.vertex_count = createInfo.vertex_count;
            geometry.triangles = new uint32_t[createInfo.triangle_count * 3];
            geometry.triangle_count = createInfo.triangle_count;
            memcpy(geometry.vertices, createInfo.vertices, sizeof(XrVector3f) * createInfo.vertex_count);
            memcpy(geometry.triangles, createInfo.triangles, sizeof(uint32_t) * createInfo.triangle_count * 3);

            // create triangle mesh
            XrTriangleMeshCreateInfoFB tmci = {XR_TYPE_TRIANGLE_MESH_CREATE_INFO_FB};
            tmci.vertexCount = createInfo.vertex_count;
            tmci.triangleCount = createInfo.triangle_count;
            tmci.vertexBuffer = geometry.vertices;
            tmci.indexBuffer = geometry.triangles;
            CHECK_POINTER_ARG_IS_NOT_NULL(xrCreateTriangleMeshFB);
            XrResult result = xrCreateTriangleMeshFB(openxr_wrapper_->GetXrSession(), &tmci, &(geometry.mesh_handle));
            if (XR_FAILED(result)) {
                PLOGE("xrCreateTriangleMeshFB at id: %d failed, ret: %d", geom_id, result);
                return XR_ERROR_VALIDATION_FAILURE;
            }
            // create geometry instance
            XrGeometryInstanceCreateInfoFB giCreateInfo = {XR_TYPE_GEOMETRY_INSTANCE_CREATE_INFO_FB};
            giCreateInfo.layer = passthrough_layers_map_[layer_id];
            giCreateInfo.mesh = geometry.mesh_handle;
            giCreateInfo.baseSpace = createInfo.base_space;
            giCreateInfo.pose = createInfo.pose;
            giCreateInfo.scale = createInfo.scale;
            CHECK_POINTER_ARG_IS_NOT_NULL(xrCreateGeometryInstanceFB);
            result = xrCreateGeometryInstanceFB(openxr_wrapper_->GetXrSession(), &giCreateInfo,
                                                &(geometry.instance_handle));
            if (XR_FAILED(result)) {
                PLOGE("xrCreateGeometryInstanceFB failed id: %d failed, ret: %d", geom_id, result);
                return XR_ERROR_VALIDATION_FAILURE;
            }

            geometry_map_[geom_id] = geometry;
            return XR_SUCCESS;
        }

        return XR_ERROR_VALIDATION_FAILURE;
    }

    void Passthrough::DestroyPassthroughGeometry(int id) {
        // check if geometry exists
        if (passthrough_system_properties_.supportsPassthrough && geometry_map_.find(id) != geometry_map_.end()) {
            auto &mesh = geometry_map_[id].mesh_handle;
            if (mesh != XR_NULL_HANDLE) {
                CHECK_POINTER_ARG_IS_NOT_NULL_VOID(xrDestroyTriangleMeshFB);
                xrDestroyTriangleMeshFB(mesh);
                mesh = XR_NULL_HANDLE;
            }
            auto &geo = geometry_map_[id].instance_handle;
            if (geo != XR_NULL_HANDLE) {
                CHECK_POINTER_ARG_IS_NOT_NULL_VOID(xrDestroyGeometryInstanceFB);
                xrDestroyGeometryInstanceFB(geo);
                geo = XR_NULL_HANDLE;
            }
            geometry_map_.erase(id);
            return;
        }
        PLOGI("DestroyPassthroughGeometry skip, please make sure passthrough geometry id: %d was created before!", id);
    }

    int Passthrough::SetGeometryTransform(int geometryId, const XrGeometryInstanceTransformFB &transformInfo) {
        // check if geometry exists
        if (passthrough_system_properties_.supportsPassthrough &&
            geometry_map_.find(geometryId) != geometry_map_.end()) {
            XrGeometryInstanceTransformFB transform = transformInfo;
            auto &geometry = geometry_map_[geometryId];
            CHECK_POINTER_ARG_IS_NOT_NULL(xrGeometryInstanceSetTransformFB);
            XrResult result = xrGeometryInstanceSetTransformFB(geometry.instance_handle, &transform);
            if (XR_FAILED(result)) {
                PLOGE("xrGeometryInstanceSetTransformFB failed id: %d failed, ret: %d", geometryId, result);
                return XR_ERROR_VALIDATION_FAILURE;
            }
            return XR_SUCCESS;
        }
        return XR_ERROR_VALIDATION_FAILURE;
    }

    Passthrough::FBPassthroughGeometry::~FBPassthroughGeometry() {
        if (vertices != nullptr && vertex_count > 0) {
            delete[] vertices;
            vertices = nullptr;
            vertex_count = 0;
        }
        if (triangles != nullptr && triangle_count > 0) {
            delete[] triangles;
            triangles = nullptr;
            triangle_count = 0;
        }

        // NOTE: destroy handles at Passthrough instance
    }
}  // namespace PVRSampleFW
