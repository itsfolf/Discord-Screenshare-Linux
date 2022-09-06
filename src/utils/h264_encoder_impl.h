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

#ifndef MODULES_VIDEO_CODING_CODECS_H264_H264_DECODER_IMPL_H_
#define MODULES_VIDEO_CODING_CODECS_H264_H264_DECODER_IMPL_H_

// Everything declared in this header is only required when WebRTC is
// build with H264 support, please do not move anything out of the
// #ifdef unless needed and tested.

#include <memory>

#include "modules/video_coding/codecs/h264/include/h264.h"

// CAVEAT: According to ffmpeg docs for avcodec_send_packet, ffmpeg requires a
// few extra padding bytes after the end of input. And in addition, docs for
// AV_INPUT_BUFFER_PADDING_SIZE says "If the first 23 bits of the additional
// bytes are not 0, then damaged MPEG bitstreams could cause overread and
// segfault."
//
// WebRTC doesn't ensure any such padding, and REQUIRES ffmpeg to be compiled
// with CONFIG_SAFE_BITSTREAM_READER, which is intended to eliminate
// out-of-bounds reads. ffmpeg docs doesn't say explicitly what effects this
// flag has on the h.264 decoder or avcodec_send_packet, though, so this is in
// some way depending on undocumented behavior. If any problems turn up, we may
// have to add an extra copy operation, to enforce padding before buffers are
// passed to ffmpeg.

#include "common_video/h264/h264_bitstream_parser.h"
extern "C" {
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

  struct AVCodecContextDeleter
  {
    void operator()(AVCodecContext *ptr) const { avcodec_free_context(&ptr); }
  };
  struct AVFrameDeleter
  {
    void operator()(AVFrame *ptr) const { av_frame_free(&ptr); }
  };
  struct AVBufferDeleter
  {
    void operator()(AVBufferRef *ptr) const { av_buffer_unref(&ptr); }
  };

  class H264EncoderImpl : public webrtc::H264Encoder
  {
  public:
    H264EncoderImpl();
    ~H264EncoderImpl() override;

    int32_t InitEncode(const webrtc::VideoCodec *codec_settings, const webrtc::VideoEncoder::Settings &encoder_settings) override;
    int32_t Release() override;

    int32_t RegisterEncodeCompleteCallback(
        webrtc::EncodedImageCallback *callback) override;

    int32_t Encode(const webrtc::VideoFrame &frame,
                   const std::vector<webrtc::VideoFrameType> *frame_types) override;

    void SetRates(const webrtc::VideoEncoder::RateControlParameters &parameters);

    EncoderInfo GetEncoderInfo() const override;

    static std::unique_ptr<H264EncoderImpl> Create();

  private:
  webrtc::VideoCodecType codec_type_;
    bool IsInitialized() const;
    int setHWFrameContext(AVCodecContext *ctx, AVBufferRef *hw_device_ctx, int width, int height);
    bool createHWContext(AVHWDeviceType type);
    // Used to allocate NV12 images if NV12 output is preferred.
    std::unique_ptr<AVCodecContext, AVCodecContextDeleter> av_context_;
    std::unique_ptr<AVFrame, AVFrameDeleter> av_frame_;

    webrtc::EncodedImageCallback *encoded_image_callback_;

    webrtc::H264BitstreamParser h264_bitstream_parser_;
    std::unique_ptr<AVBufferRef, AVBufferDeleter> hw_context_;
    webrtc::VideoCodecType findCodecType(std::string name);
    const char* findEncoderName(webrtc::VideoCodecType codec_type);
  };

} // namespace webrtc

#endif // MODULES_VIDEO_CODING_CODECS_H264_H264_DECODER_IMPL_H_
