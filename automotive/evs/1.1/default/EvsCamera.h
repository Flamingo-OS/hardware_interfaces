/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSCAMERA_H
#define ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSCAMERA_H

#include "ConfigManager.h"

#include <android/hardware/automotive/evs/1.1/IEvsCamera.h>
#include <android/hardware/automotive/evs/1.1/IEvsCameraStream.h>
#include <android/hardware/automotive/evs/1.1/IEvsDisplay.h>
#include <android/hardware/automotive/evs/1.1/types.h>
#include <ui/GraphicBuffer.h>

#include <thread>

namespace android::hardware::automotive::evs::V1_1::implementation {

// From EvsEnumerator.h
class EvsEnumerator;

class EvsCamera : public IEvsCamera {
  public:
    // Methods from ::android::hardware::automotive::evs::V1_0::IEvsCamera follow.
    Return<void> getCameraInfo(getCameraInfo_cb _hidl_cb) override;
    Return<V1_0::EvsResult> setMaxFramesInFlight(uint32_t bufferCount) override;
    Return<V1_0::EvsResult> startVideoStream(const sp<V1_0::IEvsCameraStream>& stream) override;
    Return<void> stopVideoStream() override;
    Return<void> doneWithFrame(const V1_0::BufferDesc& buffer) override;

    Return<int32_t> getExtendedInfo(uint32_t opaqueIdentifier) override;
    Return<V1_0::EvsResult> setExtendedInfo(uint32_t opaqueIdentifier,
                                            int32_t opaqueValue) override;

    // Methods from ::android::hardware::automotive::evs::V1_1::IEvsCamera follow.
    Return<void> getCameraInfo_1_1(getCameraInfo_1_1_cb _hidl_cb) override;
    Return<void> getPhysicalCameraInfo(const hidl_string& id,
                                       getPhysicalCameraInfo_cb _hidl_cb) override;
    Return<V1_0::EvsResult> pauseVideoStream() override;
    Return<V1_0::EvsResult> resumeVideoStream() override;
    Return<V1_0::EvsResult> doneWithFrame_1_1(const hidl_vec<BufferDesc>& buffer) override;
    Return<V1_0::EvsResult> setMaster() override;
    Return<V1_0::EvsResult> forceMaster(const sp<V1_0::IEvsDisplay>& display) override;
    Return<V1_0::EvsResult> unsetMaster() override;
    Return<void> getParameterList(getParameterList_cb _hidl_cb) override;
    Return<void> getIntParameterRange(CameraParam id, getIntParameterRange_cb _hidl_cb) override;
    Return<void> setIntParameter(CameraParam id, int32_t value,
                                 setIntParameter_cb _hidl_cb) override;
    Return<void> getIntParameter(CameraParam id, getIntParameter_cb _hidl_cb) override;
    Return<V1_0::EvsResult> setExtendedInfo_1_1(uint32_t opaqueIdentifier,
                                                const hidl_vec<uint8_t>& opaqueValue) override;
    Return<void> getExtendedInfo_1_1(uint32_t opaqueIdentifier,
                                     getExtendedInfo_1_1_cb _hidl_cb) override;
    Return<void> importExternalBuffers(const hidl_vec<BufferDesc>& buffers,
                                       importExternalBuffers_cb _hidl_cb) override;

    static sp<EvsCamera> Create(const char* deviceName);
    static sp<EvsCamera> Create(const char* deviceName,
                                std::unique_ptr<ConfigManager::CameraInfo>& camInfo,
                                const Stream* streamCfg = nullptr);
    EvsCamera(const EvsCamera&) = delete;
    EvsCamera& operator=(const EvsCamera&) = delete;

    virtual ~EvsCamera() override;
    void forceShutdown();  // This gets called if another caller "steals" ownership of the camera

    const CameraDesc& getDesc() { return mDescription; };

  private:
    EvsCamera(const char* id, std::unique_ptr<ConfigManager::CameraInfo>& camInfo);
    // These three functions are expected to be called while mAccessLock is held
    //
    bool setAvailableFrames_Locked(unsigned bufferCount);
    unsigned increaseAvailableFrames_Locked(unsigned numToAdd);
    unsigned decreaseAvailableFrames_Locked(unsigned numToRemove);

    void generateFrames();
    void fillTestFrame(const V1_0::BufferDesc& buff);
    void fillTestFrame(const BufferDesc& buff);
    void returnBufferLocked(const uint32_t bufferId, const buffer_handle_t memHandle);

    sp<EvsEnumerator> mEnumerator;  // The enumerator object that created this camera

    CameraDesc mDescription = {};  // The properties of this camera

    std::thread mCaptureThread;  // The thread we'll use to synthesize frames

    uint32_t mWidth = 0;   // Horizontal pixel count in the buffers
    uint32_t mHeight = 0;  // Vertical pixel count in the buffers
    uint32_t mFormat = 0;  // Values from android_pixel_format_t
    uint64_t mUsage = 0;   // Values from from Gralloc.h
    uint32_t mStride = 0;  // Bytes per line in the buffers

    sp<IEvsCameraStream> mStream = nullptr;  // The callback used to deliver each frame

    struct BufferRecord {
        buffer_handle_t handle;
        bool inUse;

        explicit BufferRecord(buffer_handle_t h) : handle(h), inUse(false){};
    };

    std::vector<BufferRecord> mBuffers;  // Graphics buffers to transfer images
    unsigned mFramesAllowed;             // How many buffers are we currently using
    unsigned mFramesInUse;               // How many buffers are currently outstanding

    enum StreamStateValues {
        STOPPED,
        RUNNING,
        STOPPING,
        DEAD,
    };
    StreamStateValues mStreamState;

    // Synchronization necessary to deconflict mCaptureThread from the main service thread
    std::mutex mAccessLock;

    // Static camera module information
    std::unique_ptr<ConfigManager::CameraInfo>& mCameraInfo;

    // For the extended info
    std::unordered_map<uint32_t, hidl_vec<uint8_t>> mExtInfo;
    std::unordered_map<CameraParam, int32_t> mParams;
};

}  // namespace android::hardware::automotive::evs::V1_1::implementation

#endif  // ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_EVSCAMERA_H
