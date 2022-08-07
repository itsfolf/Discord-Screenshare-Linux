#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "desktop_capture_source.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_frame.h"
#include "api/video/i420_buffer.h"


#include <thread>
#include <atomic>

namespace webrtc_demo {

class DesktopCapture : public DesktopCaptureSource,
                       public webrtc::DesktopCapturer::Callback,
                       public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  static DesktopCapture* Create(size_t target_fps);

  ~DesktopCapture() override;

  void StartCapture();
  void StopCapture();

 private:
  DesktopCapture();

  void Destroy();

  void OnFrame(const webrtc::VideoFrame& frame) override {}

  bool Init(size_t target_fps);

  void OnCaptureResult(webrtc::DesktopCapturer::Result result,
                       std::unique_ptr<webrtc::DesktopFrame> frame) override;

  

  std::unique_ptr<webrtc::DesktopCapturer> dc_;

  size_t fps_;

  std::unique_ptr<std::thread> capture_thread_;
  std::atomic_bool start_flag_;

  rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer_;
};
}  // namespace webrtc_demo// EXAMPLES_DESKTOP_CAPTURE_DESKTOP_CAPTURER_TEST_H_