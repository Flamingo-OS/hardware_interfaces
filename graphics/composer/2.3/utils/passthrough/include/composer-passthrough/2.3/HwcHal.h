/*
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#ifndef LOG_TAG
#warning "HwcHal.h included without LOG_TAG"
#endif

#include <type_traits>

#include <composer-hal/2.3/ComposerHal.h>
#include <composer-passthrough/2.2/HwcHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace composer {
namespace V2_3 {
namespace passthrough {

namespace detail {

using common::V1_1::RenderIntent;
using common::V1_2::ColorMode;
using common::V1_2::Dataspace;
using common::V1_2::Hdr;
using common::V1_2::PixelFormat;
using V2_1::Display;
using V2_1::Error;

// HwcHalImpl implements V2_*::hal::ComposerHal on top of hwcomposer2
template <typename Hal>
class HwcHalImpl : public V2_2::passthrough::detail::HwcHalImpl<Hal> {
   public:
    Error getPerFrameMetadataKeys_2_3(
        Display display, std::vector<IComposerClient::PerFrameMetadataKey>* outKeys) override {
        std::vector<V2_2::IComposerClient::PerFrameMetadataKey> castKeys;
        Error error = getPerFrameMetadataKeys(display, &castKeys);
        if (error != Error::NONE) {
            return error;
        }
        outKeys->clear();
        for (auto key : castKeys) {
            outKeys->push_back(static_cast<IComposerClient::PerFrameMetadataKey>(key));
        }
        return Error::NONE;
    }

    Error setLayerPerFrameMetadata_2_3(
        Display display, Layer layer,
        const std::vector<IComposerClient::PerFrameMetadata>& metadata) override {
        return setLayerPerFrameMetadata(
            display, layer,
            reinterpret_cast<const std::vector<V2_2::IComposerClient::PerFrameMetadata>&>(
                metadata));
    }

    Error setColorMode_2_3(Display display, ColorMode mode, RenderIntent intent) override {
        return setColorMode_2_2(display, static_cast<common::V1_1::ColorMode>(mode), intent);
    }

    Error getRenderIntents_2_3(Display display, ColorMode mode,
                               std::vector<RenderIntent>* outIntents) override {
        return getRenderIntents(display, static_cast<common::V1_1::ColorMode>(mode), outIntents);
    }

    Error getColorModes_2_3(Display display, hidl_vec<ColorMode>* outModes) override {
        return getColorModes_2_2(display,
                                 reinterpret_cast<hidl_vec<common::V1_1::ColorMode>*>(outModes));
    }

    Error getHdrCapabilities_2_3(Display display, hidl_vec<Hdr>* outTypes, float* outMaxLuminance,
                                 float* outMaxAverageLuminance, float* outMinLuminance) override {
        return getHdrCapabilities(display, reinterpret_cast<hidl_vec<common::V1_0::Hdr>*>(outTypes),
                                  outMaxLuminance, outMaxAverageLuminance, outMinLuminance);
    }

    Error getClientTargetSupport_2_3(Display display, uint32_t width, uint32_t height,
                                     PixelFormat format, Dataspace dataspace) override {
        return getClientTargetSupport_2_2(display, width, height,
                                          static_cast<common::V1_1::PixelFormat>(format),
                                          static_cast<common::V1_1::Dataspace>(dataspace));
    }

    Error getReadbackBufferAttributes_2_3(Display display, PixelFormat* outFormat,
                                          Dataspace* outDataspace) override {
        return getReadbackBufferAttributes(
                display, reinterpret_cast<common::V1_1::PixelFormat*>(outFormat),
                reinterpret_cast<common::V1_1::Dataspace*>(outDataspace));
    }

    Error getDisplayIdentificationData(Display display, uint8_t* outPort,
                                       std::vector<uint8_t>* outData) override {
        if (!mDispatch.getDisplayIdentificationData) {
            return Error::UNSUPPORTED;
        }

        uint32_t size = 0;
        int32_t error =
            mDispatch.getDisplayIdentificationData(mDevice, display, outPort, &size, nullptr);
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }

        std::vector<uint8_t> data(size);
        error =
            mDispatch.getDisplayIdentificationData(mDevice, display, outPort, &size, data.data());
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }

        data.resize(size);
        *outData = std::move(data);
        return Error::NONE;
    }

    Error setLayerColorTransform(Display display, Layer layer, const float* matrix) override {
        if (!mDispatch.setLayerColorTransform) {
            return Error::UNSUPPORTED;
        }
        int32_t err = mDispatch.setLayerColorTransform(mDevice, display, layer, matrix);
        return static_cast<Error>(err);
    }

    Error getDisplayedContentSamplingAttributes(
        uint64_t display, PixelFormat& format, Dataspace& dataspace,
        hidl_bitfield<IComposerClient::FormatColorComponent>& componentMask) override {
        if (!mDispatch.getDisplayedContentSamplingAttributes) {
            return Error::UNSUPPORTED;
        }
        int32_t formatRaw = 0;
        int32_t dataspaceRaw = 0;
        uint8_t componentMaskRaw = 0;
        int32_t errorRaw = mDispatch.getDisplayedContentSamplingAttributes(
            mDevice, display, &formatRaw, &dataspaceRaw, &componentMaskRaw);
        auto error = static_cast<Error>(errorRaw);
        if (error == Error::NONE) {
            format = static_cast<PixelFormat>(formatRaw);
            dataspace = static_cast<Dataspace>(dataspaceRaw);
            componentMask =
                static_cast<hidl_bitfield<IComposerClient::FormatColorComponent>>(componentMaskRaw);
        }
        return error;
    };

    Error setDisplayedContentSamplingEnabled(
        uint64_t display, IComposerClient::DisplayedContentSampling enable,
        hidl_bitfield<IComposerClient::FormatColorComponent> componentMask,
        uint64_t maxFrames) override {
        if (!mDispatch.setDisplayedContentSamplingEnabled) {
            return Error::UNSUPPORTED;
        }
        return static_cast<Error>(mDispatch.setDisplayedContentSamplingEnabled(
            mDevice, display, static_cast<int32_t>(enable), componentMask, maxFrames));
    }

    Error getDisplayedContentSample(uint64_t display, uint64_t maxFrames, uint64_t timestamp,
                                    uint64_t& frameCount, hidl_vec<uint64_t>& sampleComponent0,
                                    hidl_vec<uint64_t>& sampleComponent1,
                                    hidl_vec<uint64_t>& sampleComponent2,
                                    hidl_vec<uint64_t>& sampleComponent3) override {
        if (!mDispatch.getDisplayedContentSample) {
            return Error::UNSUPPORTED;
        }

        int32_t size[4] = {0};
        auto errorRaw = mDispatch.getDisplayedContentSample(mDevice, display, maxFrames, timestamp,
                                                            &frameCount, size, nullptr);
        if (errorRaw != HWC2_ERROR_NONE) {
            return static_cast<Error>(errorRaw);
        }

        sampleComponent0.resize(size[0]);
        sampleComponent1.resize(size[1]);
        sampleComponent2.resize(size[2]);
        sampleComponent3.resize(size[3]);
        uint64_t* samples[] = {sampleComponent0.data(), sampleComponent1.data(),
                               sampleComponent2.data(), sampleComponent3.data()};
        errorRaw = mDispatch.getDisplayedContentSample(mDevice, display, maxFrames, timestamp,
                                                       &frameCount, size, samples);
        return static_cast<Error>(errorRaw);
    }

    Error getDisplayCapabilities(
        Display display, hidl_vec<IComposerClient::DisplayCapability>* outCapabilities) override {
        if (!mDispatch.getDisplayCapabilities) {
            return Error::UNSUPPORTED;
        }

        uint32_t count = 0;
        int32_t error = mDispatch.getDisplayCapabilities(mDevice, display, &count, nullptr);
        if (error != HWC2_ERROR_NONE) {
            return static_cast<Error>(error);
        }
        outCapabilities->resize(count);
        error = mDispatch.getDisplayCapabilities(
            mDevice, display, &count,
            reinterpret_cast<std::underlying_type<IComposerClient::DisplayCapability>::type*>(
                outCapabilities->data()));
        if (error != HWC2_ERROR_NONE) {
            *outCapabilities = hidl_vec<IComposerClient::DisplayCapability>();
            return static_cast<Error>(error);
        }
        return Error::NONE;
    }

    Error setLayerPerFrameMetadataBlobs(
        Display display, Layer layer,
        std::vector<IComposerClient::PerFrameMetadataBlob>& metadata) override {
        if (!mDispatch.setLayerPerFrameMetadataBlobs) {
            return Error::UNSUPPORTED;
        }

        std::vector<IComposerClient::PerFrameMetadataKey> keys;
        std::vector<uint32_t> sizes;
        std::vector<uint8_t> blobs;

        for (auto metadataBlob : metadata) {
            keys.push_back(metadataBlob.key);
            sizes.push_back(metadataBlob.blob.size());

            int writeIndex = blobs.size();
            blobs.resize(blobs.size() + metadataBlob.blob.size());
            memcpy(blobs.data() + writeIndex, metadataBlob.blob.data(), metadataBlob.blob.size());
        }

        int32_t err = mDispatch.setLayerPerFrameMetadataBlobs(
            mDevice, display, layer, static_cast<uint32_t>(metadata.size()),
            reinterpret_cast<int32_t*>(keys.data()), reinterpret_cast<uint32_t*>(sizes.data()),
            blobs.data());
        return static_cast<Error>(err);
    }

   protected:
    bool initDispatch() override {
        if (!BaseType2_2::initDispatch()) {
            return false;
        }

        this->initOptionalDispatch(HWC2_FUNCTION_GET_DISPLAY_IDENTIFICATION_DATA,
                                   &mDispatch.getDisplayIdentificationData);
        this->initOptionalDispatch(HWC2_FUNCTION_SET_LAYER_COLOR_TRANSFORM,
                                   &mDispatch.setLayerColorTransform);
        this->initOptionalDispatch(HWC2_FUNCTION_GET_DISPLAYED_CONTENT_SAMPLING_ATTRIBUTES,
                                   &mDispatch.getDisplayedContentSamplingAttributes);
        this->initOptionalDispatch(HWC2_FUNCTION_SET_DISPLAYED_CONTENT_SAMPLING_ENABLED,
                                   &mDispatch.setDisplayedContentSamplingEnabled);
        this->initOptionalDispatch(HWC2_FUNCTION_GET_DISPLAYED_CONTENT_SAMPLE,
                                   &mDispatch.getDisplayedContentSample);
        this->initOptionalDispatch(HWC2_FUNCTION_GET_DISPLAY_CAPABILITIES,
                                   &mDispatch.getDisplayCapabilities);
        this->initOptionalDispatch(HWC2_FUNCTION_SET_LAYER_PER_FRAME_METADATA_BLOBS,
                                   &mDispatch.setLayerPerFrameMetadataBlobs);
        return true;
    }

   private:
    struct {
        HWC2_PFN_GET_DISPLAY_IDENTIFICATION_DATA getDisplayIdentificationData;
        HWC2_PFN_SET_LAYER_COLOR_TRANSFORM setLayerColorTransform;
        HWC2_PFN_GET_DISPLAYED_CONTENT_SAMPLING_ATTRIBUTES getDisplayedContentSamplingAttributes;
        HWC2_PFN_SET_DISPLAYED_CONTENT_SAMPLING_ENABLED setDisplayedContentSamplingEnabled;
        HWC2_PFN_GET_DISPLAYED_CONTENT_SAMPLE getDisplayedContentSample;
        HWC2_PFN_GET_DISPLAY_CAPABILITIES getDisplayCapabilities;
        HWC2_PFN_SET_LAYER_PER_FRAME_METADATA_BLOBS setLayerPerFrameMetadataBlobs;
    } mDispatch = {};

    using BaseType2_2 = V2_2::passthrough::detail::HwcHalImpl<Hal>;
    using BaseType2_1 = V2_1::passthrough::detail::HwcHalImpl<Hal>;
    using BaseType2_1::getHdrCapabilities;
    using BaseType2_1::mDevice;
    using BaseType2_2::getClientTargetSupport_2_2;
    using BaseType2_2::getColorModes_2_2;
    using BaseType2_2::getPerFrameMetadataKeys;
    using BaseType2_2::getReadbackBufferAttributes;
    using BaseType2_2::getRenderIntents;
    using BaseType2_2::setColorMode_2_2;
    using BaseType2_2::setLayerPerFrameMetadata;
};

}  // namespace detail

using HwcHal = detail::HwcHalImpl<hal::ComposerHal>;

}  // namespace passthrough
}  // namespace V2_3
}  // namespace composer
}  // namespace graphics
}  // namespace hardware
}  // namespace android