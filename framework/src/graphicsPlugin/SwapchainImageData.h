// Copyright (c) 2017-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

/*
 * Copyright 2024 - 2025 PICO.
 *
 * This code is a PICO modified version from OpenXR SDK distribution by The Khronos Group Inc.
 *
 * */

#ifndef PICONATIVEOPENXRSAMPLES_SWAPCHAINIMAGEDATA_H
#define PICONATIVEOPENXRSAMPLES_SWAPCHAINIMAGEDATA_H

#include <openxr/openxr.h>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace PVRSampleFW {

    /// Base interface for generic interaction with swapchain images.
    ///
    /// (Replaces IGraphicsPlugin::SwapchainImageStruct)
    class ISwapchainImageData {
    public:
        virtual ~ISwapchainImageData();

        /// Get a pointer to the first color swapchain image in the array, as a base pointer,
        /// for use in passing to `xrEnumerateSwapchainImages`.
        ///
        /// Cannot be used as an array of XrSwapchainImageBaseHeader: must first cast to a
        /// pointer to the derived/specialized type, and treat that as an array.
        /// This is an API quirk of `xrEnumerateSwapchainImages`.
        virtual XrSwapchainImageBaseHeader* GetColorImageArray() noexcept = 0;

        /// Get a pointer to the first depth swapchain image in the array, as a base pointer,
        /// for use in passing to `xrEnumerateSwapchainImages`.
        ///
        /// Cannot be used as an array of XrSwapchainImageBaseHeader: must first cast to a
        /// pointer to the derived/specialized type, and treat that as an array.
        /// This is an API quirk of `xrEnumerateSwapchainImages`.
        virtual XrSwapchainImageBaseHeader* GetDepthImageArray() noexcept = 0;

        /// If depth is being provided by an XrSwapchain, acquire and wait it, and associate it with
        /// the colorImageIndex specified.
        virtual void AcquireAndWaitDepthSwapchainImage(uint32_t colorImageIndex) = 0;

        /// Releases a depth swapchain image, if applicable
        virtual void ReleaseDepthSwapchainImage() = 0;

        /// Get image number @p colorImageIndex in the swapchain, as a base pointer. Cast it
        /// to the derived/specialized type to get the data.
        virtual XrSwapchainImageBaseHeader* GetGenericColorImage(uint32_t colorImageIndex) = 0;

        /// Get the number of swapchain image structs that are currently allocated.
        virtual uint32_t GetCapacity() const noexcept = 0;

        /// Release resources, if applicable, and clear internal storage.
        virtual void Reset() = 0;
    };

    /// A helper class used by SwapchainImageDataBase to handle the type-independent parts of dealing with
    /// a runtime-provided depth swapchain.
    class DepthSwapchainHandling {
    public:
        explicit DepthSwapchainHandling(XrSwapchain depthSwapchain);

        /// Acquire and wait a depth swapchain, and associate the depth swapchain image index with
        /// the colorImageIndex specified.
        void AcquireAndWaitDepthSwapchainImage(uint32_t colorImageIndex);

        /// Retrieve the waited depth swapchain image index associated with the given color image index.
        bool GetWaitedDepthSwapchainImageIndexFor(uint32_t colorImageIndex, uint32_t* outDepthImageIndex) const;

        /// Releases a depth swapchain image, if applicable
        void ReleaseDepthSwapchainImage();

    private:
        mutable std::mutex mutex_;
        XrSwapchain depth_swapchain_;
        // used as a fifo queue, in case somebody is actually trying to acquire more than one depth swapchain image
        std::vector<std::pair<uint32_t, uint32_t>> color_to_acquired_depthIndices_;
    };

    /// A class that is responsible for holding enumerated swapchain images in their specific types,
    /// along with their associated XrSwapchainCreateInfo.
    ///
    /// This is a base class template, extended by each graphics plugin to add API-specific functionality.
    ///
    /// It implements the generic interface @ref ISwapchainImageData
    ///
    /// @tparam SwapchainImageDerivedType The per-API OpenXR structure type based on XrSwapchainImageBaseHeader
    template <typename SwapchainImageDerivedType>
    class SwapchainImageDataBase : public ISwapchainImageData {
    protected:
        /// Constructor with no explicit depth swapchain: must call from a subclass
        ///
        /// @param derivedTypeConstant The `XrStructureType` for your specialized, API-specific
        /// swapchain image struct @p SwapchainImageDerivedType
        /// @param capacity The number of swapchain image structs to allocate.
        /// @param colorSwapchainCreateInfo The info used to create your color swapchain.
        SwapchainImageDataBase(XrStructureType derivedTypeConstant, uint32_t capacity,
                               const XrSwapchainCreateInfo& colorSwapchainCreateInfo)
            : derived_type_constant_(derivedTypeConstant)
            , color_info_(colorSwapchainCreateInfo)
            , color_swapchain_images_(capacity, GetEmptyImage())
            , depth_swapchain_images_(capacity, GetEmptyImage()) {
        }

        /// Constructor with explicit depth swapchain: must call from a subclass
        ///
        /// @param derivedTypeConstant The `XrStructureType` for your specialized, API-specific
        /// swapchain image struct @p SwapchainImageDerivedType
        /// @param capacity The number of swapchain image structs to allocate.
        /// @param colorSwapchainCreateInfo The info used to create your color swapchain.
        /// @param depthSwapchain The handle to your depth swapchain: while we won't own it,
        /// we will acquire, wait, and release images on it
        /// @param depthSwapchainCreateInfo The info used to create your depth swapchain.
        SwapchainImageDataBase(XrStructureType derivedTypeConstant, uint32_t capacity,
                               const XrSwapchainCreateInfo& colorSwapchainCreateInfo, XrSwapchain depthSwapchain,
                               const XrSwapchainCreateInfo& depthSwapchainCreateInfo)
            : runtime_depth_handling_(std::make_unique<DepthSwapchainHandling>(depthSwapchain))
            , derived_type_constant_(derivedTypeConstant)
            , color_info_(colorSwapchainCreateInfo)
            , color_swapchain_images_(capacity, GetEmptyImage())
            , depth_info_(depthSwapchainCreateInfo)
            , depth_swapchain_images_(capacity, GetEmptyImage()) {
        }

    public:
        /// Get a pointer to the first color swapchain image in the array, as a base pointer,
        /// for use in passing to `xrEnumerateSwapchainImages`.
        ///
        /// Implementation of base interface
        XrSwapchainImageBaseHeader* GetColorImageArray() noexcept override;

        /// Get a pointer to the first depth swapchain image in the array, as a base pointer,
        /// for use in passing to `xrEnumerateSwapchainImages`.
        ///
        /// Implementation of base interface
        XrSwapchainImageBaseHeader* GetDepthImageArray() noexcept override;

        /// Get the number of swapchain image structs that are currently allocated.
        ///
        /// Implementation of base interface
        uint32_t GetCapacity() const noexcept override;

        /// Access the generic `XrSwapchainImageBaseHeader` pointer for color image index @p colorImageIndex
        ///
        /// Implementation of base interface
        XrSwapchainImageBaseHeader* GetGenericColorImage(uint32_t i) override;

        /// If depth is being provided by an XrSwapchain, acquire and wait it, and associate it with
        /// the colorImageIndex specified.
        ///
        /// Implementation of base interface
        void AcquireAndWaitDepthSwapchainImage(uint32_t colorImageIndex) override;

        /// Releases a depth swapchain image, if applicable
        ///
        /// Implementation of base interface
        void ReleaseDepthSwapchainImage() override;

        /// Release resources, if applicable, and clear internal storage.
        ///
        /// If your class overrides this too (e.g. to free fallback depth buffers),
        /// be sure to call this base implementation within your override.
        ///
        /// Implementation of base interface
        void Reset() override {
            color_swapchain_images_.clear();
        }

        /// Access the fully typed swapchain image for index @p colorImageIndex
        const SwapchainImageDerivedType& GetTypedImage(uint32_t colorImageIndex) const {
            return color_swapchain_images_.at(colorImageIndex);
        }

        /// Get access to the XrSwapchainCreateInfo used to create the color swapchain.
        const XrSwapchainCreateInfo& GetCreateInfo() const noexcept {
            return color_info_;
        }

        /// Returns a pointer to the XrSwapchainCreateInfo used to create the depth swapchain,
        /// or nullptr if fallback depth textures are used instead.
        const XrSwapchainCreateInfo* GetDepthCreateInfo() const noexcept {
            if (DepthSwapchainEnabled()) {
                return &depth_info_;
            }
            return nullptr;
        }

        /// Get the depth sample count: from depth create info if it exists, otherwise from color (as fallback)
        uint32_t DepthSampleCount() const noexcept {
            if (DepthSwapchainEnabled()) {
                return depth_info_.sampleCount;
            }
            return color_info_.sampleCount;
        }

        /// True if `arraySize` in the color create info was greater than 1.
        bool HasMultipleSlices() const noexcept {
            return color_info_.arraySize > 1;
        }

        /// True if `sampleCount` in the color create info was greater than 1.
        bool IsMultisample() const noexcept {
            return color_info_.sampleCount > 1;
        }

        /// Get the width requested for the color swapchain image
        uint32_t Width() const noexcept {
            return color_info_.width;
        }

        /// Get the height requested for the color swapchain image
        uint32_t Height() const noexcept {
            return color_info_.height;
        }

        /// Get the array size requested for the color swapchain image
        uint32_t ArraySize() const noexcept {
            return color_info_.arraySize;
        }

        /// Get the sample count requested for the color swapchain image
        uint32_t SampleCount() const noexcept {
            return color_info_.sampleCount;
        }

        /// Are we using a runtime-allocated XrSwapchain for depth?
        bool DepthSwapchainEnabled() const noexcept {
            return runtime_depth_handling_ != nullptr;
        }

        /// Get the depth swapchain image as a derived type, using the index of the corresponding color swapchain image.
        SwapchainImageDerivedType GetDepthImageForColorIndex(uint32_t colorImageIndex);

    protected:
        virtual const SwapchainImageDerivedType& GetFallbackDepthSwapchainImage(uint32_t colorImageIndex) = 0;

    private:
        SwapchainImageDerivedType GetEmptyImage() noexcept {
            return SwapchainImageDerivedType{derived_type_constant_};
        }

        std::unique_ptr<DepthSwapchainHandling> runtime_depth_handling_;

        XrStructureType derived_type_constant_;
        XrSwapchainCreateInfo color_info_{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        std::vector<SwapchainImageDerivedType> color_swapchain_images_;

        XrSwapchainCreateInfo depth_info_{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        std::vector<SwapchainImageDerivedType> depth_swapchain_images_;
    };

    template <typename SwapchainImageDerivedType>
    inline XrSwapchainImageBaseHeader*
    SwapchainImageDataBase<SwapchainImageDerivedType>::GetColorImageArray() noexcept {
        return reinterpret_cast<XrSwapchainImageBaseHeader*>(color_swapchain_images_.data());
    }

    template <typename SwapchainImageDerivedType>
    inline XrSwapchainImageBaseHeader*
    SwapchainImageDataBase<SwapchainImageDerivedType>::GetDepthImageArray() noexcept {
        return reinterpret_cast<XrSwapchainImageBaseHeader*>(depth_swapchain_images_.data());
    }

    template <typename SwapchainImageDerivedType>
    inline void
    SwapchainImageDataBase<SwapchainImageDerivedType>::AcquireAndWaitDepthSwapchainImage(uint32_t colorImageIndex) {
        if (!DepthSwapchainEnabled()) {
            return;
        }
        runtime_depth_handling_->AcquireAndWaitDepthSwapchainImage(colorImageIndex);
    }

    template <typename SwapchainImageDerivedType>
    inline void SwapchainImageDataBase<SwapchainImageDerivedType>::ReleaseDepthSwapchainImage() {
        if (!DepthSwapchainEnabled()) {
            return;
        }
        runtime_depth_handling_->ReleaseDepthSwapchainImage();
    }

    template <typename SwapchainImageDerivedType>
    inline SwapchainImageDerivedType
    SwapchainImageDataBase<SwapchainImageDerivedType>::GetDepthImageForColorIndex(uint32_t colorImageIndex) {
        if (DepthSwapchainEnabled()) {
            uint32_t depthImageIndex;
            if (!runtime_depth_handling_->GetWaitedDepthSwapchainImageIndexFor(colorImageIndex, &depthImageIndex)) {
                throw std::runtime_error("No depth image waited and associated with this color image!");
            }
            return depth_swapchain_images_[depthImageIndex];
        }
        return this->GetFallbackDepthSwapchainImage(colorImageIndex);
    }

    template <typename SwapchainImageDerivedType>
    inline uint32_t SwapchainImageDataBase<SwapchainImageDerivedType>::GetCapacity() const noexcept {
        return static_cast<uint32_t>(color_swapchain_images_.size());
    }

    template <typename SwapchainImageDerivedType>
    inline XrSwapchainImageBaseHeader*
    SwapchainImageDataBase<SwapchainImageDerivedType>::GetGenericColorImage(uint32_t colorImageIndex) {
        return reinterpret_cast<XrSwapchainImageBaseHeader*>(&color_swapchain_images_.at(colorImageIndex));
    }

    /// A collection of @ref ISwapchainImageData derived objects, in their fully-specialized type.
    /// Generic `XrSwapchainImageBaseHeader` pointers map to an ISwapchainImageData-based type and an image index.
    ///
    /// Functionality used by all graphics plugins, so it has been extracted into this common code
    template <typename SwapchainImageData>
    class SwapchainImageDataMap {
    public:
        static_assert(std::is_base_of<ISwapchainImageData, SwapchainImageData>::value,
                      "Your swapchain image data type must implement the interface");

        /// Take ownership of @p data, and add every swapchain image in @p data
        /// to our internal map of base pointers to image data and index.
        void Adopt(std::unique_ptr<SwapchainImageData>&& data) {
            const auto size = data->GetCapacity();
            for (uint32_t colorImageIndex = 0; colorImageIndex < size; ++colorImageIndex) {
                // Map every swapchainImage base pointer to this typed pointer
                swapchain_image_data_map_[data->GetGenericColorImage(colorImageIndex)] =
                        std::make_pair(data.get(), colorImageIndex);
            }
            image_datas_.emplace_back(std::move(data));
        }

        /// Given a base pointer for a color swapchain image, look up the image data object and swapchain
        /// image index associated with it.
        /// If not found for some reason, the pointer will be null.
        std::pair<SwapchainImageData*, uint32_t>
        GetDataAndIndexFromBasePointer(const XrSwapchainImageBaseHeader* basePointer) const {
            std::pair<SwapchainImageData*, uint32_t> ret{};

            auto it = swapchain_image_data_map_.find(basePointer);
            if (it != swapchain_image_data_map_.end()) {
                ret = it->second;
            }
            return ret;
        }

        /// Call Reset on all known SwapchainImageData, then clear internal storage.
        void Reset() {
            for (auto& imageData : image_datas_) {
                imageData->Reset();
            }
            Clear();
        }

        /// Empty internal containers, without first calling reset on their contents.
        void Clear() {
            swapchain_image_data_map_.clear();
            image_datas_.clear();
        }

    private:
        /// Owns the image data, placed in order of adoption.
        std::vector<std::unique_ptr<SwapchainImageData>> image_datas_;

        /// Associates base pointers for color swapchain images with the corresponding image data and index.
        std::map<const XrSwapchainImageBaseHeader*, std::pair<SwapchainImageData*, uint32_t>> swapchain_image_data_map_;
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_SWAPCHAINIMAGEDATA_H
