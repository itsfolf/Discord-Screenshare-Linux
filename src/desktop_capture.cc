#include "desktop_capture.h"

#include "modules/desktop_capture/desktop_capture_options.h"
#include "rtc_base/logging.h"
#include "libyuv.h"

namespace webrtc_demo
{
  DesktopCapture::DesktopCapture() : dc_(nullptr), start_flag_(false) {}

  DesktopCapture::~DesktopCapture()
  {
    Destroy();
  }

  void DesktopCapture::Destroy()
  {
    StopCapture();

    if (!dc_)
      return;

    dc_.reset(nullptr);
  }

  DesktopCapture *DesktopCapture::Create(size_t target_fps)
  {
    std::unique_ptr<DesktopCapture> dc(new DesktopCapture());
    if (!dc->Init(target_fps))
    {
      RTC_LOG(LS_WARNING) << "Failed to create DesktopCapture(fps = "
                          << target_fps << ")";
      return nullptr;
    }
    return dc.release();
  }

  bool DesktopCapture::Init(size_t target_fps)
  {
    auto opt = webrtc::DesktopCaptureOptions::CreateDefault();
    opt.set_allow_pipewire(true);
    dc_ = webrtc::DesktopCapturer::CreateScreenCapturer(opt);

    dc_->Start(this);
    dc_->SelectSource(0);

    fps_ = target_fps;

    RTC_LOG(LS_INFO) << "Init DesktopCapture finish fps = " << fps_ << "";

    return true;
  }

  void DesktopCapture::OnCaptureResult(
      webrtc::DesktopCapturer::Result result,
      std::unique_ptr<webrtc::DesktopFrame> frame)
  {
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

    if (result != webrtc::DesktopCapturer::Result::SUCCESS)
    {
      RTC_LOG(LS_ERROR) << "Capture frame failed, result: " << result;
      return;
    }
    int width = frame->size().width();
    int height = frame->size().height();


    if (!i420_buffer_.get() ||
        i420_buffer_->width() * i420_buffer_->height() < width * height)
    {
      i420_buffer_ = webrtc::I420Buffer::Create(width, height);
    }

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
