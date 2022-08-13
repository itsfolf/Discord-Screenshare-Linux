#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/desktop_capture/desktop_and_cursor_composer.h"

#include "rtc_base/logging.h"
#include "Hook.h"
#include "Hooker.h"

using namespace std;

namespace LinuxFix
{
    unique_ptr<HookerContext> hookCtx = make_unique<HookerContext>();

    using HookFn = std::function<bool(void *)>;
    std::map<std::string, HookFn> hookMap = {
        {"_ZN7discord5media17ScreenshareHelper14GetSourceCountENS_19DesktopCapturerTypeE", [](void *funcPtr)
         {
             return CreateHook((void *)funcPtr, (void *)GetSourceCount);
         }}};

    vector<string>* Hooker::GetRequiredFunctions()
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
            int i = it->second((void*)pointer);
            using namespace rtc;
            RTC_LOG_V(i == 0 ? LS_VERBOSE : LS_ERROR)
                << (i == 0 ? "Successfully hooked" : "Failed to hook") << " function " << it->first << " at " << pointer;
        }

        return 0;
    }

    namespace
    {
        void *GetSourceCount(void *type)
        {
            return (void *)1;
        }
    }
}