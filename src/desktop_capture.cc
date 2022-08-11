#include "desktop_capture.h"

#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/desktop_capture/desktop_and_cursor_composer.h"
#include "rtc_base/logging.h"
#include "libyuv.h"
#include "rtc_base/thread.h"
namespace webrtc_demo
{
  rtc::Thread *GlobalCapturerThread()
  {
    static auto result = []
    {
      auto thread = rtc::Thread::Create();
      thread->SetName("WebRTC-DesktopCapturer", nullptr);
      thread->Start();
      return thread;
    }();
    return result.get();
  };

  rtc::Thread *GlobalLoopThread()
  {
    static auto result = []
    {
      auto thread = rtc::Thread::Create();
      thread->SetName("WebRTC-Loop", nullptr);
      thread->Start();
      return thread;
    }();
    return result.get();
  };

  DesktopCapture::DesktopCapture() : dc_(nullptr), _isRunning(false), _thread(GlobalCapturerThread()) {}

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

  DesktopCapture *DesktopCapture::Create(std::atomic_bool useComposer)
  {
    std::unique_ptr<DesktopCapture> dc(new DesktopCapture());
    if (!dc->Init(&useComposer))
    {
      RTC_LOG(LS_WARNING) << "Failed to create DesktopCapture";
      return nullptr;
    }
    return dc.release();
  }

  bool DesktopCapture::Init(std::atomic_bool useComposer)
  {
    auto opt = webrtc::DesktopCaptureOptions::CreateDefault();
    opt.set_allow_pipewire(true);
    dc_ = webrtc::DesktopCapturer::CreateScreenCapturer(opt);

    if (useComposer) {
       dc_ = std::make_unique<webrtc::DesktopAndCursorComposer>(std::move(dc_), opt);
    }
   

    if (!dc_)
    {
      return false;
    }

    //dc_->SelectSource(0);
    //dc_->Start(this);

    RTC_LOG(LS_INFO) << "Init DesktopCapture finish fps = " << 0 << "";

    return true;
  }

  void DesktopCapture::StartCapture(webrtc::DesktopCapturer::Callback *cb)
  {

    if (_isRunning)
    {
      RTC_LOG(LS_WARNING) << "Capture already been running...";
      return;
    }
    RTC_LOG(LS_INFO) << "Starting capture...";
    dc_->Start(cb);
    _isRunning = true;
    GlobalLoopThread()->PostTask([this](){
      g_main_loop_run(loop);
    });
    //loop();
  }

  void DesktopCapture::CaptureFrame()
  {
    if (!dc_ || !_isRunning)
    {
      return;
    }

    dc_->CaptureFrame();
  }

  void DesktopCapture::StopCapture()
  {
    RTC_LOG(LS_INFO) << "Stopping capture...";
    _isRunning = false;
    g_main_loop_quit(loop);
  }

} // namespace webrtc_demo
