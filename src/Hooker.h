#include <glib-object.h>

#include "DiscordInjector.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/desktop_capture/mouse_cursor_monitor.h"
#include "modules/desktop_capture/desktop_and_cursor_composer.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/sdp_video_format.h"
#include "rtc_base/thread.h"
#include "utils/h264_encoder_impl.h"
namespace LinuxFix
{
    class Hooker
    {
    public:
        static std::vector<std::string> *GetRequiredFunctions();
        static int Install(DiscordVoiceModule *voice_module);
    };

    namespace
    {
        struct HookerContext
        {
            GMainLoop *loop = g_main_loop_new(NULL, FALSE);
            rtc::Thread *mainLoopThread;
            webrtc::DesktopCaptureOptions options;
        };

        void CreateDefaultOptionsHk();
        void *GetSourceCount(void *type);
        std::unique_ptr<webrtc::DesktopCapturer> CreateScreenCapturerHk(webrtc::DesktopCaptureOptions options);
        std::unique_ptr<webrtc::DesktopCapturer> CreateWindowCapturerHk(webrtc::DesktopCaptureOptions options);
        std::unique_ptr<webrtc::MouseCursorMonitor> CreateMouseCursorMonitorHk(webrtc::DesktopCaptureOptions options);
        int InitEncodeHk(const webrtc::VideoCodec *codec_settings, const webrtc::VideoEncoder::Settings &encoder_settings);
    }
}