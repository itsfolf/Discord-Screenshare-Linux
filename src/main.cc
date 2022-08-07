#include <napi.h>
#include "desktop_capture.h"
#include "rtc_base/logging.h"
#include "LogSinkImpl.h"

using namespace Napi;
using namespace std;

std::unique_ptr<LogSinkImpl> _logSink;
std::unique_ptr<webrtc_demo::DesktopCapture> capturer(webrtc_demo::DesktopCapture::Create(15));

Napi::String Method(const Napi::CallbackInfo &info)
{
  rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);
  rtc::LogMessage::SetLogToStderr(false);

  FilePath logPath;
  logPath.data = std::string("debug");
  _logSink = std::make_unique<LogSinkImpl>(logPath);
  rtc::LogMessage::AddLogToStream(_logSink.get(), rtc::LS_VERBOSE);

  capturer->StartCapture();

  Napi::Env env = info.Env();
  return Napi::String::New(env, "a");
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  exports.Set(Napi::String::New(env, "Init"),
              Napi::Function::New(env, Method));
  return exports;
}

NODE_API_MODULE(addon, Init)