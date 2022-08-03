#include <napi.h>
#include "testing.h"

using namespace Napi;
using namespace std;


Napi::String Method(const Napi::CallbackInfo &info)
{
  auto discord = DiscordPawgers();
  discord.Init();
  Napi::Env env = info.Env();
  return Napi::String::New(env, "a");
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set(Napi::String::New(env, "Discordaudio"),
              Napi::Function::New(env, Method));
  return exports;
}

NODE_API_MODULE(addon, Init)