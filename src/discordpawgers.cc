#include <napi.h>
#include "desktop_capture.h"
#include "rtc_base/logging.h"
#include "LogSinkImpl.h"

using namespace Napi;
using namespace std;


Napi::String Method(const Napi::CallbackInfo &info)
{
  FilePath logPath;
  logPath.data = std::string("debug");
  auto _logSink = std::make_unique<LogSinkImpl>(logPath);
  rtc::LogMessage::LogToDebug(rtc::LS_INFO);
  rtc::LogMessage::SetLogToStderr(false);
	rtc::LogMessage::AddLogToStream(_logSink.get(), rtc::LS_INFO);
  std::unique_ptr<webrtc_demo::DesktopCapture> capturer(webrtc_demo::DesktopCapture::Create(15,0));
  capturer->StartCapture();
  //discord.Init();
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