#include "DiscordInjector.h"

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
        struct HookerContext {
            webrtc::DesktopCaptureOptions options;
        };

        void CreateDefaultOptionsHk();
        void *GetSourceCount(void *type);
        std::unique_ptr<webrtc::DesktopCapturer> CreateScreenCapturerHk(webrtc::DesktopCaptureOptions options);
    }
}