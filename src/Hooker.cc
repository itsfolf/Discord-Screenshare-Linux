#include "rtc_base/logging.h"
#include "Hook.h"
#include "Hooker.h"
#include "utils/threads.h"
#include "utils/CustomEncoderFactory.h"

using namespace std;
using namespace webrtc;

namespace LinuxFix
{
    unique_ptr<HookerContext> hookCtx = make_unique<HookerContext>();

    using HookFn = function<bool(void *)>;
    map<string, HookFn> hookMap = {
        {"_ZN6webrtc15DesktopCapturer20CreateScreenCapturerERKNS_21DesktopCaptureOptionsE", [](void *funcPtr)
         {
             return CreateHook((void *)funcPtr, (void *)CreateScreenCapturerHk);
         }},
        {"_ZN6webrtc22CroppingWindowCapturer14CreateCapturerERKNS_21DesktopCaptureOptionsE", [](void *funcPtr)
         {
             return CreateHook((void *)funcPtr, (void *)CreateWindowCapturerHk);
         }},
        {"_ZN6webrtc18MouseCursorMonitor6CreateERKNS_21DesktopCaptureOptionsE", [](void *funcPtr)
         {
             return CreateHook((void *)funcPtr, (void *)CreateMouseCursorMonitorHk);
         }},
        {"_ZN7discord5media17ScreenshareHelper14GetSourceCountENS_19DesktopCapturerTypeE", [](void *funcPtr)
         {
             return CreateHook((void *)funcPtr, (void *)GetSourceCount);
         }},
        {"_ZN6webrtc15H264EncoderImpl10InitEncodeEPKNS_10VideoCodecERKNS_12VideoEncoder8SettingsE", [](void *funcPtr) {
            return CreateHook((void *)funcPtr, (void *)InitEncodeHk);
         }}};

    vector<string> *Hooker::GetRequiredFunctions()
    {
        static vector<string> required_functions;
        for (auto it = hookMap.begin(); it != hookMap.end(); ++it)
        {
            required_functions.push_back(it->first);
        }
        return &required_functions;
    }

    int Hooker::Install(DiscordVoiceModule *voice_module)
    {
        auto pointer_map = voice_module->pointer_map;

        for (auto it = hookMap.begin(); it != hookMap.end(); ++it)
        {
            uint64_t pointer = pointer_map.at(it->first);
            int i = it->second((void *)pointer);
            using namespace rtc;
            RTC_LOG_V(i == 0 ? LS_VERBOSE : LS_ERROR)
                << (i == 0 ? "Successfully hooked" : "Failed to hook") << " function " << it->first << " at " << pointer;
        }

        return 0;
    }

    namespace
    {
        unique_ptr<H264Encoder> encoder = H264EncoderImpl::Create();
        void CreateDefaultOptionsHk()
        {
            hookCtx->options = DesktopCaptureOptions::CreateDefault();
            hookCtx->options.set_allow_pipewire(true);
        }

        void *GetSourceCount(void *type)
        {
            return (void *)1;
        }

        unique_ptr<DesktopCapturer> CreateScreenCapturerHk(DesktopCaptureOptions options)
        {
            CreateDefaultOptionsHk();
            if (!g_main_loop_is_running(hookCtx->loop))
            {
                GlobalLoopThread()->PostTask([]()
                                             { g_main_loop_run(hookCtx->loop); });
            }
            return DesktopCapturer::CreateScreenCapturer(hookCtx->options);
        }

        unique_ptr<DesktopCapturer> CreateWindowCapturerHk(DesktopCaptureOptions options)
        {
            CreateDefaultOptionsHk();
            if (!g_main_loop_is_running(hookCtx->loop))
            {
                GlobalLoopThread()->PostTask([]()
                                             { g_main_loop_run(hookCtx->loop); });
            }
            return DesktopCapturer::CreateWindowCapturer(hookCtx->options);
        }

        unique_ptr<MouseCursorMonitor> CreateMouseCursorMonitorHk(DesktopCaptureOptions options)
        {
            return MouseCursorMonitor::Create(hookCtx->options);
        }

        int InitEncodeHk(const webrtc::VideoCodec *codec_settings, const webrtc::VideoEncoder::Settings &encoder_settings)
        {
            RTC_LOG(LS_INFO) << "InitEncodeHk";
            return encoder->InitEncode(codec_settings, encoder_settings);
        }

    }
}