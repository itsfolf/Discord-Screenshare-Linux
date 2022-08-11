#include <napi.h>
#include <unistd.h>
#include "frida-core.h"

namespace LinuxFix
{

    class DiscordInjector
    {
    public:
        DiscordInjector(Napi::Promise::Deferred promise) : _promise(promise) {};
        void Inject(pid_t pid);
    private:
        Napi::Promise::Deferred _promise;
        void _Inject(pid_t pid);
    };

}