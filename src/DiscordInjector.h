#pragma once

#include <napi.h>
#include <unistd.h>
#include <link.h>
#include <map>

namespace LinuxFix
{
    struct DiscordVoiceModule
    {
        const char *path;
        ElfW(Addr) base_address;
        ElfW(Addr) preferred_address;
        std::map<std::string, uint64_t> pointer_map;
    };
    struct ElfSymbolDetails
    {
        const char *name;
        ElfW(Addr) address;
        size_t size;
        char type;
        char bind;
        uint16_t section_header_index;
    };

    class DiscordInjector
    {
    public:
        DiscordInjector(Napi::Promise::Deferred promise) : _promise(promise){};
        void Inject(pid_t pid);

    private:
        Napi::Promise::Deferred _promise;
        int FindFunctionPointers(DiscordVoiceModule *voice_module);
        void _Inject(pid_t pid);
    };

}