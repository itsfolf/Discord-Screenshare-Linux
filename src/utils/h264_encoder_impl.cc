/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

// Everything declared/defined in this header is only required when WebRTC is
// build with H264 support, please do not move anything out of the
// #ifdef unless needed and tested.

#include "h264_encoder_impl.h"

#include <algorithm>
#include <limits>
#include <memory>

#include "api/video/color_space.h"
#include "api/video/i010_buffer.h"
#include "api/video/i420_buffer.h"
#include "common_video/include/video_frame_buffer.h"
#include "modules/video_coding/codecs/h264/h264_color_space.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "system_wrappers/include/field_trial.h"
#include "system_wrappers/include/metrics.h"
#include "libyuv/convert.h"
#include "vaapi_encode.h"

#define DEFAULT_FPS \
  (AVRational) { 25, 1 }

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
}
namespace LinuxFix
{
  using namespace webrtc;
  H264EncoderImpl::H264EncoderImpl()
      : encoded_image_callback_(nullptr){};

  H264EncoderImpl::~H264EncoderImpl()
  {
    Release();
  }

  std::unique_ptr<H264EncoderImpl> H264EncoderImpl::Create()
  {
    return absl::make_unique<H264EncoderImpl>();
  }

  static void avlog_cb(void *data, int level, const char *szFmt, va_list varg)
  {
    va_list vl2;
    char line[1024];
    static int print_prefix = 1;

    va_copy(vl2, varg);
    av_log_default_callback(data, level, szFmt, varg);
    av_log_format_line(data, level, szFmt, vl2, line, sizeof(line), &print_prefix);
    va_end(vl2);
    RTC_LOG(LS_INFO) << line;
  }

  int32_t H264EncoderImpl::InitEncode(const webrtc::VideoCodec *codec_settings, const webrtc::VideoEncoder::Settings &encoder_settings)
  {
    RTC_LOG(LS_INFO) << "H264EncoderImpl::InitEncode " << codec_settings->codecType;
    av_log_set_callback(avlog_cb);
    if (!codec_settings)
    {
      return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    if (codec_settings->maxFramerate == 0 || codec_settings->width < 1 || codec_settings->height < 1)
    {
      // return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    codec_type_ = codec_settings->codecType;

    // Release necessary in case of re-initializing.
    int32_t ret = Release();
    if (ret != WEBRTC_VIDEO_CODEC_OK)
    {
      return ret;
    }
    RTC_DCHECK(!av_context_);

    // Save dimensions
    int width = codec_settings->width;
    int height = codec_settings->height;

    if (!createHWContext(AV_HWDEVICE_TYPE_VAAPI))
    {
      // return WEBRTC_VIDEO_CODEC_ERROR;
    }

    /*
       // init HW device
       if (isDeviceNeeded())
       {
         auto hardware_type_str = CodecUtils::ConvertHardwareTypeToString(hardware_type_);
         AVHWDeviceType type = findHWDeviceType(hardware_type_str.c_str());
         if (type == AV_HWDEVICE_TYPE_NONE || !createHWContext(type))
         {
           return WEBRTC_VIDEO_CODEC_ERROR;
         }
       }*/

    const char *codec_name = "h264_vaapi"; // findEncoderName(codec_type_);
    if (!codec_name)
    {
      RTC_LOG(LS_ERROR) << "Device/codec type combination is not supported";
      Release();
      return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // find codec
    const AVCodec *codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec)
    {
      RTC_LOG(LS_ERROR) << "Could not find " << codec_name
                        << " encoder";
      Release();
      return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // Initialize AVCodecContext.
    av_context_.reset(avcodec_alloc_context3(codec));
    av_context_->width = width;
    av_context_->height = height;
    av_context_->time_base = (AVRational){1, 90000};
    av_context_->sample_aspect_ratio = (AVRational){1, 1};
    av_context_->pix_fmt = AV_PIX_FMT_VAAPI;
    // fill configuration
    AVRational fps = DEFAULT_FPS;
    av_context_->framerate = fps;
    av_context_->max_b_frames = 0;
    av_context_->gop_size = 250;
    av_context_->bit_rate = codec_settings->maxBitrate * 1000;
    av_context_->rc_max_rate = codec_settings->maxBitrate * 1000;
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "rc_mode", "CBR", 0);
    //
    int profile = FF_PROFILE_H264_CONSTRAINED_BASELINE;
    if (profile != FF_PROFILE_UNKNOWN)
    {
      av_context_->profile = profile;
    }
    //
    int level = 31;
    if (level != -1)
    {
      av_context_->level = level;
    }

    /* set hw_frames_ctx for encoder's AVCodecContext */
    // if (isDeviceNeeded())
    //{
    if (setHWFrameContext(av_context_.get(), hw_context_.get(), width, height) < 0)
    {
      RTC_LOG(LS_ERROR) << "Failed to set hwframe context";
      Release();
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    //}

    // open codec
    int res = avcodec_open2(av_context_.get(), codec, &opts);
    if (res < 0)
    {
      RTC_LOG(LS_ERROR) << "avcodec_open2 error: " << res;
      Release();
      return WEBRTC_VIDEO_CODEC_ERROR;
    }

    RTC_LOG(LS_INFO) << "H264EncoderImpl::InitEncode";
    return WEBRTC_VIDEO_CODEC_OK;
  }

  int32_t H264EncoderImpl::Release()
  {
    av_context_.reset();
    av_frame_.reset();
    return WEBRTC_VIDEO_CODEC_OK;
  }

  int32_t H264EncoderImpl::RegisterEncodeCompleteCallback(
      EncodedImageCallback *callback)
  {
    encoded_image_callback_ = callback;
    return WEBRTC_VIDEO_CODEC_OK;
  }

  int32_t H264EncoderImpl::Encode(const VideoFrame &input_frame,
                                  const std::vector<VideoFrameType> *frame_types)
  {
    if (!IsInitialized())
    {
      return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (!encoded_image_callback_)
    {
      RTC_LOG(LS_WARNING)
          << "Configure() has been called, but a callback function "
             "has not been set with RegisterEncodeCompleteCallback()";
      return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    rtc::scoped_refptr<I420BufferInterface> frame_buffer =
        input_frame.video_frame_buffer()->ToI420();
    if (!frame_buffer)
    {
      RTC_LOG(LS_ERROR) << "Failed to convert "
                        << VideoFrameBufferTypeToString(
                               input_frame.video_frame_buffer()->type())
                        << " image to I420. Can't encode frame.";
      return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
    }
    RTC_CHECK(frame_buffer->type() == VideoFrameBuffer::Type::kI420 ||
              frame_buffer->type() == VideoFrameBuffer::Type::kI420A);

    std::unique_ptr<AVFrame, AVFrameDeleter> hw_frame(av_frame_alloc());
    if (/*TODO ishw*/ true)
    {
      int res = av_hwframe_get_buffer(av_context_->hw_frames_ctx, hw_frame.get(), 0);
      if (res < 0)
      {
        RTC_LOG(LS_ERROR) << "av_hwframe_get_buffer error";
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        RTC_LOG(LS_ERROR) << "Error code: " << av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, res);
        return WEBRTC_VIDEO_CODEC_ERROR;
      }
      res = av_hwframe_transfer_data(hw_frame.get(), av_frame_.get(), 0);
      if (res < 0)
      {
        RTC_LOG(LS_ERROR) << "av_hwframe_transfer_data error";
        char buffer[AV_ERROR_MAX_STRING_SIZE];
        RTC_LOG(LS_ERROR) << "Error code: " << av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, res);
        return WEBRTC_VIDEO_CODEC_ERROR;
      }
    }
    else
    {
      hw_frame->data[0] = (uint8_t *)frame_buffer->DataY();
      hw_frame->data[1] = (uint8_t *)frame_buffer->DataU();
      hw_frame->data[2] = (uint8_t *)frame_buffer->DataV();
      hw_frame->linesize[0] = frame_buffer->StrideY();
      hw_frame->linesize[1] = frame_buffer->StrideU();
      hw_frame->linesize[2] = frame_buffer->StrideV();
      hw_frame->width = frame_buffer->width();
      hw_frame->height = frame_buffer->height();
      hw_frame->format = AV_PIX_FMT_NV12; // TODO: use correct format
    }
    hw_frame->pts = input_frame.timestamp();

    // decide if keyframe required
    if (frame_types)
    {
      for (auto frame_type : *frame_types)
      {
        if (frame_type == webrtc::VideoFrameType::kVideoFrameKey)
        {
          hw_frame->pict_type = AV_PICTURE_TYPE_I;
          break;
        }
      }
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;

    int result = avcodec_send_frame(av_context_.get(), hw_frame.get());
    if (result < 0)
    {
      RTC_LOG(LS_ERROR) << "avcodec_send_frame error";
      char buffer[AV_ERROR_MAX_STRING_SIZE];
      RTC_LOG(LS_ERROR) << "Error code: " << av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, result);
      return WEBRTC_VIDEO_CODEC_ERROR;
    }

    result = avcodec_receive_packet(av_context_.get(), &packet);
    if (result < 0)
    {
      char buffer[AV_ERROR_MAX_STRING_SIZE];
      RTC_LOG(LS_ERROR) << "Error code: " << av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, result);
      return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // write packet
    webrtc::EncodedImage encodedFrame;
    encodedFrame.SetEncodedData(webrtc::EncodedImageBuffer::Create(packet.data, packet.size));
    encodedFrame._encodedHeight = input_frame.height();
    encodedFrame._encodedWidth = input_frame.width();
    encodedFrame.SetTimestamp(packet.pts);
    encodedFrame._frameType = (packet.flags & AV_PKT_FLAG_KEY) ? webrtc::VideoFrameType::kVideoFrameKey
                                                               : webrtc::VideoFrameType::kVideoFrameDelta;

    // generate codec specific info
    webrtc::CodecSpecificInfo info;
    memset(&info, 0, sizeof(info));
    info.codecType = codec_type_;
    if (codec_type_ == webrtc::kVideoCodecVP8)
    {
      info.codecSpecific.VP8.nonReference = false;
      info.codecSpecific.VP8.temporalIdx = webrtc::kNoTemporalIdx;
      info.codecSpecific.VP8.layerSync = false;
      info.codecSpecific.VP8.keyIdx = webrtc::kNoKeyIdx;
    }

    // send
    const auto res = encoded_image_callback_->OnEncodedImage(encodedFrame, &info);
    if (res.error != webrtc::EncodedImageCallback::Result::Error::OK)
    {
      RTC_LOG(LS_ERROR) << "OnEncodedImage error";
      av_packet_unref(&packet);
      return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // Stop referencing it, possibly freeing `input_frame`.
    av_packet_unref(&packet);

    return WEBRTC_VIDEO_CODEC_OK;
  }

  bool H264EncoderImpl::IsInitialized() const
  {
    return av_context_ != nullptr;
  }

  int H264EncoderImpl::setHWFrameContext(AVCodecContext *ctx, AVBufferRef *hw_device_ctx, int width, int height)
  {
    AVBufferRef *hw_frames_ref;
    AVHWFramesContext *frames_ctx = NULL;
    int err = 0;

    if (!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx)))
    {
      RTC_LOG(LS_ERROR) << "Failed to create hardware frame context";
      return -1;
    }
    frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
    frames_ctx->format = AV_PIX_FMT_VAAPI;
    frames_ctx->sw_format = AV_PIX_FMT_NV12;
    frames_ctx->width = width;
    frames_ctx->height = height;
    frames_ctx->initial_pool_size = 20;
    if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0)
    {
      RTC_LOG(LS_ERROR) << "Failed to initialize hardware frame context";
      char buffer[AV_ERROR_MAX_STRING_SIZE];
      RTC_LOG(LS_ERROR) << "Error code: " << av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, err);
      av_buffer_unref(&hw_frames_ref);
      return err;
    }
    ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if (!ctx->hw_frames_ctx)
      err = AVERROR(ENOMEM);

    av_buffer_unref(&hw_frames_ref);
    return err;
  }

  void H264EncoderImpl::SetRates(const webrtc::VideoEncoder::RateControlParameters &parameters)
  {
    if (!IsInitialized())
    {
      RTC_LOG(LS_WARNING) << "SetRates() while not initialized";
      return;
    }

    if (parameters.framerate_fps < 1.0)
    {
      RTC_LOG(LS_WARNING) << "Unsupported framerate (must be >= 1.0";
      return;
    }
    av_context_->framerate = av_d2q(parameters.framerate_fps, 65535);
    VAAPIEncodeContext *ctx = (VAAPIEncodeContext *)av_context_->priv_data;
    if (ctx->rc_params.bits_per_second != parameters.bitrate.get_sum_bps())
    {
      RTC_LOG(LS_INFO) << "SetRates " << ctx->rc_params.bits_per_second << " -> "
                       << parameters.bitrate.get_sum_bps();
    }
    ctx->rc_params.bits_per_second = parameters.bitrate.get_sum_bps();
  }

  bool H264EncoderImpl::createHWContext(AVHWDeviceType type)
  {
    AVBufferRef *context = nullptr;

    int result = av_hwdevice_ctx_create(&context, type, NULL, NULL, 0);
    if (result < 0)
    {
      RTC_LOG(LS_WARNING) << "Failed to create specified HW device";
      char buffer[AV_ERROR_MAX_STRING_SIZE];
      RTC_LOG(LS_ERROR) << "Error code: " << av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, result);
      return false;
    }

    RTC_DCHECK(context);
    hw_context_.reset(context);
    return true;
  }

  webrtc::VideoEncoder::EncoderInfo H264EncoderImpl::GetEncoderInfo() const
  {
    static EncoderInfo info;
    info.supports_native_handle = false;
    info.is_hardware_accelerated = true;
    info.implementation_name = "FFmpeg";
    info.has_trusted_rate_controller = true;
    info.scaling_settings = VideoEncoder::ScalingSettings::kOff;
    info.supports_simulcast = false;
    return info;
  }

  webrtc::VideoCodecType H264EncoderImpl::findCodecType(std::string name)
  {
    if (name == cricket::kVp8CodecName)
    {
      return webrtc::kVideoCodecVP8;
    }
    else if (name == cricket::kVp9CodecName)
    {
      return webrtc::kVideoCodecVP9;
    }
    else if (name == cricket::kH264CodecName)
    {
      return webrtc::kVideoCodecH264;
    }
    else
    {
      return webrtc::kVideoCodecGeneric;
    }
  }

  const char *H264EncoderImpl::findEncoderName(webrtc::VideoCodecType codec_type)
  {
    if (true) // hardware_type_ == CodecUtils::HW_TYPE_VAAPI)
    {
      if (codec_type == webrtc::kVideoCodecH264)
      {
        return "h264_vaapi";
      }
      else if (codec_type == webrtc::kVideoCodecVP8)
      {
        return "vp8_vaapi";
      }
      else
      {
        return nullptr;
      }
    }
    else if (false) // hardware_type_ == CodecUtils::HW_TYPE_VIDEOTOOLBOX)
    {
      if (codec_type == webrtc::kVideoCodecH264)
      {
        return "h264_videotoolbox";
      }
      else
      {
        return nullptr;
      }
    }
    else
    {
      return nullptr;
    }
  }

} // namespace webrtc
