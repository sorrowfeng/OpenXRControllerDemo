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

#ifndef PICONATIVEOPENXRSAMPLES_PASSTHROUGH_H
#define PICONATIVEOPENXRSAMPLES_PASSTHROUGH_H

#include "IOpenXRExtensionPlugin.h"
#include "CheckUtils.h"
#include <unordered_map>

namespace PVRSampleFW {

    /**
    * OpenXR WG reference doc：https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_FB_passthrough
    */
    class Passthrough : public IOpenXRExtensionPlugin {
    public:
        struct FBPassthroughCompLayer {
            XrCompositionLayerPassthroughFB passthrough_comp_layer;
            int id{-1};
            XrPassthroughLayerPurposeFB purpose_type;
        };

        struct FBPassthroughGeometry {
            FBPassthroughGeometry() = default;
            ~FBPassthroughGeometry();

            int id{-1};
            int layer_id;
            XrTriangleMeshFB mesh_handle{XR_NULL_HANDLE};
            XrGeometryInstanceFB instance_handle{XR_NULL_HANDLE};
            XrVector3f *vertices{nullptr};
            int vertex_count{0};
            uint32_t *triangles{nullptr};
            int triangle_count{0};
            XrGeometryInstanceTransformFB transform;
        };

        struct FBPassthroughGeometryCreateInfo {
            int geom_id{-1};
            // mesh info
            const XrVector3f *vertices{nullptr};
            int vertex_count{0};
            const uint32_t *triangles{nullptr};
            int triangle_count{0};
            // geometry instance info
            int layer_id{-1};
            XrSpace base_space{XR_NULL_HANDLE};
            XrPosef pose;
            XrVector3f scale;
        };

    public:
        Passthrough() = default;

        virtual ~Passthrough() {
        }

        /**
         * Switch a regular vst on or off.
         *
         * @note: This is a complete black box function, which means if you just need a simple
         * passthrough layer that can overlay virtual scenes, then just use it! You no longer need
         * to worry about the creation, configuration, and state management of various handles
         * related to passthrough. It is very simple and brainless. But it also means losing some
         * flexibility.
         *
         * @param value true to on, false to off
         * @return 0 is success, the others are failed
         */
        int EnableRegularVideoSeeThrough(bool value);

        /**
         * Initialize function.
         *
         * @attention need to test
         *
         * @brief It help to create passthrough handle and complete other initialization.
         *
         * @return 0 is success, the others are failed
         */
        int Initialize();

        /**
         * Pause.
         *
         * @return true is success, false is failed
         */
        bool Pause();

        /**
         * Start.
         *
         * @return true is success, false is failed
         */
        bool Start();

        /**
         * Create a passthrough layer with specific purpose.
         *
         * @attention id must be positive or zero;
         *
         * @param id The unique ID of the layer. You are not allowed to create different layers
         * with the same id. If you do this, call will be returned directly.
         * @param purposeType purpose of this layer
         * @return true is success (including duplicate cases), false is failed
         */
        bool CreatePassthroughLayer(int id, XrPassthroughLayerPurposeFB purposeType);

        /**
         * Destroy a passthrough layer with specific id.
         * @param id
         */
        void DestroyPassthroughLayer(int id);

        /**
         * Pause a passthrough layer with specific id.
         * @param id id
         * @return true is success, false is failed
         */
        bool PausePassthroughLayer(int id);

        /**
         * Resume a passthrough layer with specific id.
         * @param id id
         * @return true is success, false is failed
         */
        bool ResumePassthroughLayer(int id);

        /**
         * Enable/disable a passthrough layer with specific id.
         *
         * @note: only enabled layer participate in synthesis
         * @param id id
         * @param value true is on, false is off
         * @return true is success, false is failed
         */
        bool EnablePassthroughLayer(int id, bool value);

        /**
         * Set FB style for a passthrough layer with specific id.
         *
         * @param id id
         * @param style style info
         * @return 0 is success, the others are failed
         */
        int SetPassthroughLayerFBStyle(int id, const XrPassthroughStyleFB &style);

        /**
         * Set lut for a passthrough layer with specific id.
         *
         * @param id id of layer
         * @param data lut data
         * @param width width of lut
         * @param height height of lut
         * @param row row
         * @param col col
         * @return 0 is success, the others are failed
         */
        int SetPassthroughLayerPicoLUT(int id, uint8_t **data, int width, int height, int row, int col);

        /**
         * Create a geometry instance at specific passthrough layer.
         *
         * @param createInfo info needed when creating
         * @return 0 is success, the others are failed
         */
        int CreatePassthroughGeometry(const FBPassthroughGeometryCreateInfo &createInfo);

        /**
         * Destroy a geometry instance with specific id.
         *
         * @param id id of geometry instance
         */
        void DestroyPassthroughGeometry(int id);

        /**
         * Set transform info of a passthrough geometry instance
         *
         * @param geometryId id of a passthrough geometry
         * @param transformInfo transform info
         * @return 0 is success, the others are failed
         */
        int SetGeometryTransform(int geometryId, const XrGeometryInstanceTransformFB &transformInfo);

        std::vector<std::string> GetRequiredExtensions() const override;

        bool OnInstanceCreate() override;

        bool OnInstanceDestroy() override;

        bool OnEventHandlerSetup() override;

        bool OnSessionCreate() override;

        bool OnSystemGet(XrSystemProperties *configProperties) override;

        bool OnPreEndFrame(std::vector<XrCompositionLayerBaseHeader *> *layers) override;

    public:
        PFN_DECLARE(xrCreatePassthroughFB);
        PFN_DECLARE(xrDestroyPassthroughFB);
        PFN_DECLARE(xrPassthroughStartFB);
        PFN_DECLARE(xrPassthroughPauseFB);
        PFN_DECLARE(xrCreatePassthroughLayerFB);
        PFN_DECLARE(xrDestroyPassthroughLayerFB);
        PFN_DECLARE(xrPassthroughLayerSetStyleFB);
        PFN_DECLARE(xrPassthroughLayerPauseFB);
        PFN_DECLARE(xrPassthroughLayerResumeFB);
        PFN_DECLARE(xrCreateGeometryInstanceFB);
        PFN_DECLARE(xrDestroyGeometryInstanceFB);
        PFN_DECLARE(xrGeometryInstanceSetTransformFB);

        PFN_DECLARE(xrCreateTriangleMeshFB);
        PFN_DECLARE(xrDestroyTriangleMeshFB);
        PFN_DECLARE(xrTriangleMeshGetVertexBufferFB);
        PFN_DECLARE(xrTriangleMeshGetIndexBufferFB);
        PFN_DECLARE(xrTriangleMeshBeginUpdateFB);
        PFN_DECLARE(xrTriangleMeshEndUpdateFB);
        PFN_DECLARE(xrTriangleMeshBeginVertexBufferUpdateFB);
        PFN_DECLARE(xrTriangleMeshEndVertexBufferUpdateFB);

    private:
        XrPassthroughFB passthrough_FB_{XR_NULL_HANDLE};
        XrSystemPassthroughPropertiesFB passthrough_system_properties_ = {XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES_FB};
        XrSystemPassthroughProperties2FB passthrough_system_properties2_FB_ = {
                XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES2_FB};
        bool passthrough_support_color_;
        bool passthrough_support_depth_;
        bool is_initialized_ = false;
        std::unordered_map<int, bool> enabled_layers_map_;
        std::unordered_map<int, FBPassthroughCompLayer> comp_layers_map_;
        std::unordered_map<int, XrPassthroughLayerFB> passthrough_layers_map_;
        std::unordered_map<int, FBPassthroughGeometry> geometry_map_;
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_PASSTHROUGH_H
