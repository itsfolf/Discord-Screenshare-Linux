#include "modules/desktop_capture/desktop_capturer.h"
#include "rtc_base/thread.h"

rtc::Thread *GlobalCapturerThread();

class CaptureScheduler
{
public:
    CaptureScheduler() : _thread(GlobalCapturerThread())
    {
    }

    void runAsync(std::function<void()> method)
    {
        _thread->PostTask(std::move(method));
    }
    void runDelayed(int64_t delayMs, std::function<void()> method)
    {
        _thread->PostDelayedTask(std::move(method), webrtc::TimeDelta::Millis(delayMs));
    }

private:
    rtc::Thread *_thread;
};

class DiscordPawgers
{
public:
    void Init();

private:
    CaptureScheduler _scheduler = CaptureScheduler();
    std::unique_ptr<webrtc::DesktopCapturer> _capturer;
    std::shared_ptr<bool> _timerGuard = std::make_shared<bool>(true);;

    void Test();
    void loop();
};