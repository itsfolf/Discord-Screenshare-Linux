#include <napi.h>
#include "rtc_base/logging.h"
#include "LogSinkImpl.h"
#include "DiscordInjector.h"

using namespace Napi;

std::unique_ptr<LogSinkImpl> _logSink;

Napi::Value Inject(const Napi::CallbackInfo &info)
{
  guint pid = 0;
  if (info.Length() > 0 && info[0].IsNumber())
  {
    pid = info[0].ToNumber().Uint32Value();
  }
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  auto injector = std::make_unique<LinuxFix::DiscordInjector>(deferred);
  injector->Inject(pid);
  return deferred.Promise();
}

void Initialize(const Napi::CallbackInfo &info)
{
  rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);
  rtc::LogMessage::LogThreads(true);
  std::string _logPath = "/tmp/linuxfix.log";
  if (info.Length() > 0 && info[0].IsString())
  {
    _logPath = info[0].As<String>().Utf8Value() + "/linuxfix.log";
  }
  _logSink = std::make_unique<LogSinkImpl>(_logPath);
  rtc::LogMessage::AddLogToStream(_logSink.get(), rtc::LS_VERBOSE);
  RTC_LOG(LS_VERBOSE) << "Initialized";
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set(Napi::String::New(env, "init"),
              Napi::Function::New(env, Initialize));
  exports.Set(Napi::String::New(env, "inject"),
              Napi::Function::New(env, Inject));
  return exports;
}

NODE_API_MODULE(addon, Init);