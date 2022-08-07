#include <napi.h>
#include "desktop_capture.h"
#include "rtc_base/logging.h"
#include "LogSinkImpl.h"

using namespace Napi;

std::unique_ptr<LogSinkImpl> _logSink;
webrtc_demo::DesktopCapture *capturer;

Napi::String Method(const Napi::CallbackInfo &info)
{
  if (!capturer)
  {
    capturer = webrtc_demo::DesktopCapture::Create(15);
  }
  capturer->StartCapture();

  Napi::Env env = info.Env();
  return Napi::String::New(env, "a");
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);
  rtc::LogMessage::SetLogToStderr(false);

  FilePath logPath;
  logPath.data = std::string("debug");
  _logSink = std::make_unique<LogSinkImpl>(logPath);
  rtc::LogMessage::AddLogToStream(_logSink.get(), rtc::LS_VERBOSE);
  exports.Set(Napi::String::New(env, "Init"),
              Napi::Function::New(env, Method));
  return exports;
}

NODE_API_MODULE(addon, Init)