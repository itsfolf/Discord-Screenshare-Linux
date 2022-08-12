#include <napi.h>
#include <unistd.h>
#include <link.h>
#include <map>

#include "frida-core.h"

using namespace std;

namespace LinuxFix
{
    struct DiscordVoiceModule {
        const char *path;
        ElfW(Addr) base_address;
        ElfW(Addr) preferred_address;
    };
    struct ElfSymbolDetails
    {
        const gchar *name;
        ElfW(Addr) address;
        gsize size;
        char type;
        char bind;
        guint16 section_header_index;
    };

    class DiscordInjector
    {
    public:
        DiscordInjector(Napi::Promise::Deferred promise) : _promise(promise){};
        void Inject(pid_t pid);

    private:
        Napi::Promise::Deferred _promise;
        bool FindFunctionPointers(DiscordVoiceModule *voice_module, map<string, intptr_t> *function_map);
        void _Inject(pid_t pid);
    };

}