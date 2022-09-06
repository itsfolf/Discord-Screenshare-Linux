//
// Created by anba8005 on 26/02/2020.
//

#ifndef NDI_WEBRTC_PEER_WORKER_CUSTOMENCODERFACTORY_H
#define NDI_WEBRTC_PEER_WORKER_CUSTOMENCODERFACTORY_H

#include <vector>
#include <atomic>
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "rtc_base/system/rtc_export.h"

class CustomEncoderFactory : public webrtc::VideoEncoderFactory {
public:
    std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(const webrtc::SdpVideoFormat &format) override;

    std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

    static std::unique_ptr<CustomEncoderFactory> Create();

    explicit CustomEncoderFactory();

    virtual ~CustomEncoderFactory() override;
};


#endif //NDI_WEBRTC_PEER_WORKER_CUSTOMENCODERFACTORY_H
