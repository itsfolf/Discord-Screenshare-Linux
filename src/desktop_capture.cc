#include "desktop_capture.h"

#include "modules/desktop_capture/desktop_capture_options.h"
#include "rtc_base/logging.h"
#include "libyuv.h"

namespace webrtc_demo
{
  DesktopCapture::DesktopCapture() : dc_(nullptr), start_flag_(false) {}

  DesktopCapture::~DesktopCapture()
  {
    Destory();
  }

  void DesktopCapture::Destory()
  {
    StopCapture();

    if (!dc_)
      return;

    dc_.reset(nullptr);
  }

  DesktopCapture *DesktopCapture::Create(size_t target_fps,
                                         size_t capture_screen_index)
  {
    std::unique_ptr<DesktopCapture> dc(new DesktopCapture());
    if (!dc->Init(target_fps, capture_screen_index))
    {
      RTC_LOG(LS_WARNING) << "Failed to create DesktopCapture(fps = "
                          << target_fps << ")";
      return nullptr;
    }
    return dc.release();
  }

  bool DesktopCapture::Init(size_t target_fps, size_t capture_screen_index)
  {
    // 窗口
    // dc_ = webrtc::DesktopCapturer::CreateWindowCapturer(
    // webrtc::DesktopCaptureOptions::CreateDefault());
    //桌面
    //	webrtc::DesktopCaptureOptions result;
    //	result.set_allow_directx_capturer(true);
    auto opt = webrtc::DesktopCaptureOptions::CreateDefault();
    opt.set_allow_pipewire(true);
    dc_ = webrtc::DesktopCapturer::CreateWindowCapturer(opt);
    // webrtc::DesktopCapturer::CreateScreenCapturer(result);
    // DesktopCapturer::CreateScreenCapturer( DesktopCaptureOptions::CreateDefault());

    webrtc::DesktopCapturer::SourceList sources;
    dc_->GetSourceList(&sources);
    if (capture_screen_index > sources.size())
    {
      RTC_LOG(LS_WARNING) << "The total sources of screen is " << sources.size()
                          << ", but require source of index at "
                          << capture_screen_index;
      return false;
    }

    // RTC_CHECK(dc_->SelectSource(0));
    window_title_ = sources[capture_screen_index].title;
    fps_ = target_fps;

    RTC_LOG(LS_INFO) << "Init DesktopCapture finish window_title = " << window_title_ << " , fps = " << fps_ << "";
    // Start new thread to capture
    return true;
  }
  // FILE *out_file_ptr = fopen("desktop.yuv", "wb+");
  void DesktopCapture::OnCaptureResult(
      webrtc::DesktopCapturer::Result result,
      std::unique_ptr<webrtc::DesktopFrame> frame)
  {
    RTC_LOG(LS_INFO) << "new Frame";

    static auto timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    static size_t cnt = 0;

    cnt++;
    auto timestamp_curr = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::system_clock::now().time_since_epoch())
                              .count();
    if (timestamp_curr - timestamp > 1000)
    {
      RTC_LOG(LS_INFO) << "FPS: " << cnt;
      cnt = 0;
      timestamp = timestamp_curr;
    }

    // Convert DesktopFrame to VideoFrame
    if (result != webrtc::DesktopCapturer::Result::SUCCESS)
    {
      RTC_LOG(LS_ERROR) << "Capture frame faiiled, result: " << result;
      return;
    }
    int width = frame->size().width();
    int height = frame->size().height();
    // int half_width = (width + 1) / 2;

    if (!i420_buffer_.get() ||
        i420_buffer_->width() * i420_buffer_->height() < width * height)
    {
      i420_buffer_ = webrtc::I420Buffer::Create(width, height);
    }
    /*if (width == 1080 && height == 1920)
    {
      fwrite(frame->data(), 1, width * height * 4, out_file_ptr);
      fflush(out_file_ptr);
    }*/

    libyuv::ConvertToI420(frame->data(), 0, i420_buffer_->MutableDataY(),
                          i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
                          i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
                          i420_buffer_->StrideV(), 0, 0, width, height, width,
                          height, libyuv::kRotate0, libyuv::FOURCC_ARGB);

    DesktopCaptureSource::OnFrame(
        webrtc::VideoFrame(i420_buffer_, 0, 0, webrtc::kVideoRotation_0));
  }

  void DesktopCapture::StartCapture()
  {

    if (start_flag_)
    {
      RTC_LOG(LS_WARNING) << "Capture already been running...";
      return;
    }
    RTC_LOG(LS_INFO) << "Starting capture...";

    start_flag_ = true;

    // Start new thread to capture
    capture_thread_.reset(new std::thread([this]()
                                          {
    RTC_LOG(LS_INFO) << "Starting capture...";
    dc_->Start(this);
    
    dc_->SelectSource(0);

    while (start_flag_)
    {
      dc_->CaptureFrame();
      std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps_));
    } }));
  }

  void DesktopCapture::StopCapture()
  {
    RTC_LOG(LS_INFO) << "Stopping capture...";
    start_flag_ = false;

    if (capture_thread_ && capture_thread_->joinable())
    {
      capture_thread_->join();
    }
  }

} // namespace webrtc_demo
