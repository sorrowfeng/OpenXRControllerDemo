// Copyright (c) 2017-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

/*
 * Copyright 2024 - 2025 PICO.
 *
 * This code is a PICO modified version from OpenXR SDK distribution by The Khronos Group Inc.
 *
 * */

#ifndef PICONATIVEOPENXRSAMPLES_IXRGRAPHICSPLUGIN_H
#define PICONATIVEOPENXRSAMPLES_IXRGRAPHICSPLUGIN_H

#include "util/Common.h"
#include "Scene.h"
#include "RGBAImage.h"
#include "SwapchainImageData.h"
#include "IXrProgram.h"
#include "IGltfBuilder.h"

namespace PVRSampleFW {
    class IXrGraphicsPlugin {
    public:
        virtual ~IXrGraphicsPlugin() = default;

        /// Returns a string describing the platform.
        /// May be called regardless of initialization state.
        /// Example returned string: "OpenGLES"
        virtual std::string DescribeGraphics() const = 0;

        /// OpenXR instance-level extensions required by this graphics.
        virtual std::vector<std::string> GetInstanceExtensionsRequiredByGraphics() const = 0;

        /// Create an instance of this graphics api for the provided instance and systemId.
        virtual void InitializeGraphicsDevice(XrInstance instance, XrSystemId systemId, IXrProgram* program) = 0;

        /// Shuts down the device initialized by InitializeGraphicsDevice.
        virtual void ShutdownGraphicsDevice() = 0;

        /// Clear any memory associated with swapchains, particularly auto-created accompanying depth buffers.
        virtual void ClearSwapchainCache() = 0;

        /// Flush any pending graphics commands.
        virtual void Flush() = 0;

        /// Copy the image to the swapchain image.
        virtual void CopyRGBAImage(const XrSwapchainImageBaseHeader* /*swapchainImage*/, uint32_t /*arraySlice*/,
                                   const Conformance::RGBAImage& /*image*/) = 0;

        /// Update the graphics plugin with the current configuration.
        virtual void UpdateConfigurationsAtGraphics(const std::shared_ptr<struct Configurations>& config) = 0;

        /// Set the background color, do clear the color buffer
        virtual void SetBackgroundColor(std::array<float, 4> color) = 0;

        /// Select the preferred swapchain format from the list of available formats.
        virtual int64_t SelectColorSwapchainFormat(const std::vector<int64_t>& runtimeFormats) const = 0;

        /// Select the preferred swapchain format from the list of available formats.
        virtual int64_t SelectDepthSwapchainFormat(const std::vector<int64_t>& runtimeFormats) const = 0;

        /// Select the preferred swapchain format from the list of available formats.
        virtual int64_t SelectMotionVectorSwapchainFormat(const std::vector<int64_t>& runtimeFormats) const = 0;

        /// Select the preferred swapchain format.
        virtual int64_t GetSRGBA8Format() const = 0;

        /// Allocate space for the swapchain image structures. These are different for each graphics API. The returned
        /// pointers are valid for the lifetime of the graphics plugin.
        // TODO: to be deprecated when we use the ISwapchainImageData interface.
        virtual std::vector<XrSwapchainImageBaseHeader*>
        AllocateSwapchainImageStructs(uint32_t capacity, const XrSwapchainCreateInfo& swapchainCreateInfo) = 0;

        /// Allocates an object owning (among other things) an array of XrSwapchainImage* in a portable way and
        /// returns an **observing** pointer to an interface providing generic access to the associated pointers.
        /// (The object remains owned by the graphics plugin, and will be destroyed on @ref ShutdownDevice())
        /// This is all for the purpose of being able to call the xrEnumerateSwapchainImages function
        /// in a platform-independent way. The user of this must not use the images beyond @ref ShutdownDevice()
        ///
        /// Example usage:
        ///
        /// ```c++
        /// ISwapchainImageData * p = graphicsPlugin->AllocateSwapchainImageData(3, swapchainCreateInfo);
        /// xrEnumerateSwapchainImages(swapchain, 3, &count, p->GetColorImageArray());
        /// ```
        virtual ISwapchainImageData* AllocateSwapchainImageData(size_t size,
                                                                const XrSwapchainCreateInfo& swapchainCreateInfo) = 0;

        /// Allocates an object owning (among other things) an array of XrSwapchainImage* in a portable way and
        /// returns an **observing** pointer to an interface providing generic access to the associated pointers.
        ///
        /// Signals that we will use a depth swapchain allocated by the runtime, instead of a fallback depth
        /// allocated by the plugin.
        virtual ISwapchainImageData* AllocateSwapchainImageDataWithDepthSwapchain(
                size_t size, const XrSwapchainCreateInfo& colorSwapchainCreateInfo, XrSwapchain depthSwapchain,
                const XrSwapchainCreateInfo& depthSwapchainCreateInfo) = 0;

        /// Clears a slice to an arbitrary color. Must be called before rendering to the image, since it may
        /// also reset internal state.
        virtual void ClearImageSlice(const XrSwapchainImageBaseHeader* colorSwapchainImage, uint32_t imageArrayIndex,
                                     XrColor4f color) = 0;

        /// Clears to the background color which varies depending on the environment blend mode that is active.
        virtual void ClearImageSlice(const XrSwapchainImageBaseHeader* colorSwapchainImage,
                                     uint32_t imageArrayIndex = 0) = 0;

        /// Get the graphics binding header for session creation.
        virtual const XrBaseInStructure* GetGraphicsBinding() const = 0;

        /// Get recommended number of sub-data element samples in view (recommendedSwapchainSampleCount)
        /// if supported by the graphics plugin. A supported value otherwise.
        virtual uint32_t GetSupportedSwapchainSampleCount(const XrViewConfigurationView& view) {
            return view.recommendedSwapchainSampleCount;
        }

        /// Get the PBR resources
        virtual Pbr::IGltfBuilder* GetPbrResources() = 0;

        /// Render scene to the provided swapchain image for projection layer
        virtual void RenderProjectionView(const XrCompositionLayerProjectionView& layerView,
                                          const XrSwapchainImageBaseHeader* swapchainImage, int64_t swapchainFormat,
                                          const std::vector<Scene>& scenes) = 0;

        /// TODO: add functions for rendering to different layers
        /*
        // Render scene to the provided swapchain image for quad layer
        virtual void RenderQuadView(const XrCompositionLayerQuad& layerQuad, const XrSwapchainImageBaseHeader* swapchainImage,
                                int64_t swapchainFormat, const std::vector<Scene>& scenes_) = 0;

        // Render scene to the provided swapchain image for cylinder layer
        virtual void RenderCylinderView(const XrCompositionLayerCylinder& layerCylinder, const XrSwapchainImageBaseHeader* swapchainImage,
                                int64_t swapchainFormat, const std::vector<Scene>& scenes_) = 0;

        // Render scene to the provided swapchain image for cube layer
        virtual void RenderCubeView(const XrCompositionLayerCube& layerCube, const XrSwapchainImageBaseHeader* swapchainImage,
                                int64_t swapchainFormat, const std::vector<Scene>& scenes_) = 0;

        // Render scene to the provided swapchain image for Equirect layer
        virtual void RenderEquirectView(const XrCompositionLayerEquirect& layerEquirect, const XrSwapchainImageBaseHeader* swapchainImage,
                                int64_t swapchainFormat, const std::vector<Scene>& scenes_) = 0;

        // Render scene to the provided swapchain image for pass-throughFB layer
        virtual void RenderPassThroughFBView(const XrCompositionLayerPassThroughFB& layerPassThroughFB, const XrSwapchainImageBaseHeader* swapchainImage,
                                int64_t swapchainFormat, const std::vector<Scene>& scenes_) = 0;
        */
    };
}  // namespace PVRSampleFW
#endif  //PICONATIVEOPENXRSAMPLES_IXRGRAPHICSPLUGIN_H
