#include <napi.h>
#include <unistd.h>
#include "frida-core.h"

namespace LinuxFix
{
    struct ElfSymbolDetails
    {
        const gchar *name;
        intptr_t address;
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
        void _Inject(pid_t pid);
    };

}