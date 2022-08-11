#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "desktop_capture_source.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_frame.h"
#include "api/video/i420_buffer.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "rtc_base/thread.h"
#include <glib-object.h>

#include <thread>
#include <atomic>

namespace webrtc_demo
{

  class DesktopCapture : public DesktopCaptureSource
  {
  public:
    static DesktopCapture *Create(std::atomic_bool useComposer);

    ~DesktopCapture() override;

    void StartCapture(webrtc::DesktopCapturer::Callback *cb);
    void StopCapture();
    void CaptureFrame();

  private:
    DesktopCapture();

    void Destroy();

    bool Init(std::atomic_bool useComposer);
  
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    std::unique_ptr<webrtc::DesktopCapturer> dc_;
    webrtc::DesktopCaptureOptions opt_;
    rtc::Thread *_thread;
    
    std::atomic_bool _isRunning;

    rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer_;
  };
} // namespace webrtc_demo// EXAMPLES_DESKTOP_CAPTURE_DESKTOP_CAPTURER_TEST_H_