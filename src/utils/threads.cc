#include "threads.h"

namespace LinuxFix
{
    rtc::Thread *GlobalLoopThread()
    {
        static auto result = []
        {
            auto thread = rtc::Thread::Create();
            thread->SetName("LinuxFix-GLib-EventLoop", nullptr);
            thread->Start();
            return thread;
        }();
        return result.get();
    };

    rtc::Thread *GlobalCaptureThread()
    {
        static auto result = []
        {
            auto thread = rtc::Thread::Create();
            thread->SetName("WebRTC-AudioCapturer", nullptr);
            thread->Start();
            return thread;
        }();
        return result.get();
    };
}
