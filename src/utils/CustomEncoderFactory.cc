//
// Created by anba8005 on 26/02/2020.
//

#include "CustomEncoderFactory.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "media/base/media_constants.h"
#include "absl/strings/match.h"

extern "C"
{
#include "libavutil/rational.h"
}

#include <iostream>
#include "h264_encoder_impl.h"
#include "rtc_base/logging.h"

CustomEncoderFactory::CustomEncoderFactory()
{
}

CustomEncoderFactory::~CustomEncoderFactory()
{
}

std::unique_ptr<webrtc::VideoEncoder> CustomEncoderFactory::CreateVideoEncoder(const webrtc::SdpVideoFormat &format)
{
    RTC_LOG(LS_INFO) << "CustomEncoderFactory::CreateVideoEncoder: " << format.name;
    //if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
    //    return webrtc::VP8Encoder::Create();
    //if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
    //    return webrtc::VP9Encoder::Create(cricket::VideoCodec(format));
    //if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName))
    return LinuxFix::H264EncoderImpl::Create();
    // return webrtc::H264Encoder::Create(cricket::VideoCodec(format));
    //return nullptr;
}

std::vector<webrtc::SdpVideoFormat> CustomEncoderFactory::GetSupportedFormats() const
{
    std::vector<webrtc::SdpVideoFormat> supported_codecs;
    supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
    for (const webrtc::SdpVideoFormat &format : webrtc::SupportedVP9Codecs())
        supported_codecs.push_back(format);
    for (const webrtc::SdpVideoFormat &format : webrtc::SupportedH264Codecs())
        supported_codecs.push_back(format);
    RTC_LOG(LS_INFO) << "CustomEncoderFactory::GetSupportedFormats: " << supported_codecs.size();
    return supported_codecs;
}

std::unique_ptr<CustomEncoderFactory> CustomEncoderFactory::Create()
{
    return absl::make_unique<CustomEncoderFactory>();
}
