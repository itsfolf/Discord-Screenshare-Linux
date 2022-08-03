#include <iostream>
#include <fstream>
#include <napi.h>
#include <stdio.h>
#include <link.h>
#include <link.h>
#include <string>
#include <vector>
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "api/video/video_frame.h"
#include "api/video/i420_buffer.h"
#include "testing.h"
#include "api/video/video_sink_interface.h"

using std::cout;
using std::endl;
using std::ofstream;
using std::string;
using std::to_string;

struct DesktopSize
{
    int width = 0;
    int height = 0;
};

rtc::Thread *GlobalCapturerThread()
{
    static auto result = []
    {
        auto thread = rtc::Thread::Create();
        thread->SetName("WebRTC-DesktopCapturer", nullptr);
        thread->Start();
        return thread;
    }();
    return result.get();
}

class SourceFrameCallbackImpl : public webrtc::DesktopCapturer::Callback
{
public:
    SourceFrameCallbackImpl(DesktopSize size, int fps);

    void OnCaptureResult(
        webrtc::DesktopCapturer::Result result,
        std::unique_ptr<webrtc::DesktopFrame> frame) override;
    void setOutput(
        std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink);
    void setSecondaryOutput(
        std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink);
    void setOnFatalError(std::function<void()>);
    void setOnPause(std::function<void(bool)>);

private:
    rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer_;
    std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> _sink;
    std::shared_ptr<
        rtc::VideoSinkInterface<webrtc::VideoFrame>>
        _secondarySink;
    DesktopSize size_;
    std::function<void()> _onFatalError;
    std::function<void(bool)> _onPause;
};

SourceFrameCallbackImpl::SourceFrameCallbackImpl(DesktopSize size, int fps)
    : size_(size)
{
}

void SourceFrameCallbackImpl::OnCaptureResult(
    webrtc::DesktopCapturer::Result result,
    std::unique_ptr<webrtc::DesktopFrame> frame)
{
    printf("\nFrame Result: %d\n", result);
    const auto failed = (result != webrtc::DesktopCapturer::Result::SUCCESS) || !frame || frame->size().equals({1, 1});
    if (failed)
    {
        if (result == webrtc::DesktopCapturer::Result::ERROR_PERMANENT)
        {
            if (_onFatalError)
            {
                _onFatalError();
            }
        }
        else if (_onPause)
        {
            _onPause(true);
        }
        return;
    }
    else if (_onPause)
    {
        _onPause(false);
    }
    /*
         const auto frameSize = frame->size();
         auto fittedSize = (frameSize.width() >= size_.width * 2
             || frameSize.height() >= size_.height * 2)
             ? DesktopSize{ frameSize.width() / 2, frameSize.height() / 2 }
             : DesktopSize{ frameSize.width(), frameSize.height() };

         fittedSize.width -= (fittedSize.width % 4);
         fittedSize.height -= (fittedSize.height % 4);

         const auto outputSize = webrtc::DesktopSize{
             fittedSize.width,
             fittedSize.height
         };

         webrtc::BasicDesktopFrame outputFrame{ outputSize };

       const auto outputRect = webrtc::DesktopRect::MakeSize(outputSize);

       const auto outputRectData = outputFrame.data() +
             outputFrame.stride() * outputRect.top() +
         webrtc::DesktopFrame::kBytesPerPixel * outputRect.left();


       libyuv::ARGBScale(
         frame->data(),
         frame->stride(),
         frame->size().width(),
         frame->size().height(),
             outputRectData,
             outputFrame.stride(),
             outputSize.width(),
             outputSize.height(),
         libyuv::kFilterBilinear);

       int width = outputFrame.size().width();
       int height = outputFrame.size().height();
       int stride_y = width;
       int stride_uv = (width + 1) / 2;

       if (!i420_buffer_
             || i420_buffer_->width() != width
             || i420_buffer_->height() != height) {
         i420_buffer_ = webrtc::I420Buffer::Create(
                 width,
                 height,
                 stride_y,
                 stride_uv,
                 stride_uv);
       }

       int i420Result = libyuv::ConvertToI420(
             outputFrame.data(),
         width * height,
         i420_buffer_->MutableDataY(), i420_buffer_->StrideY(),
         i420_buffer_->MutableDataU(), i420_buffer_->StrideU(),
         i420_buffer_->MutableDataV(), i420_buffer_->StrideV(),
         0, 0,
         width, height,
         width, height,
         libyuv::kRotate0,
         libyuv::FOURCC_ARGB);


       assert(i420Result == 0);
       (void)i420Result;
       webrtc::VideoFrame nativeVideoFrame = webrtc::VideoFrame(
         i420_buffer_,
         webrtc::kVideoRotation_0,
             webrtc::Clock::GetRealTimeClock()->CurrentTime().us());
       if (const auto sink = _sink.get()) {
         _sink->OnFrame(nativeVideoFrame);
       }
       if (const auto sink = _secondarySink.get()) {
         sink->OnFrame(nativeVideoFrame);
       }*/
}

void SourceFrameCallbackImpl::setOutput(
    std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink)
{
    printf("sink");
    _sink = std::move(sink);
}

void SourceFrameCallbackImpl::setOnFatalError(std::function<void()> error)
{
    printf("F");
    _onFatalError = error;
}
void SourceFrameCallbackImpl::setOnPause(std::function<void(bool)> pause)
{
    printf("Paused %d", pause);
    _onPause = pause;
}

void SourceFrameCallbackImpl::setSecondaryOutput(
    std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> sink)
{
    printf("sink2");
    _secondarySink = std::move(sink);
}

void DiscordPawgers::Init()
{
    Test();
}

void DiscordPawgers::Test()
{
    DesktopSize size;
    size.height = 720;
    size.width = 1280;
    SourceFrameCallbackImpl _callback = SourceFrameCallbackImpl(size, 30);
    ofstream myfile;
    myfile.open("/tmp/example.txt");
    webrtc::DesktopCaptureOptions options = webrtc::DesktopCaptureOptions::CreateDefault();
    options.set_allow_pipewire(true);
    _capturer = webrtc::DesktopCapturer::CreateWindowCapturer(options);
    auto list = webrtc::DesktopCapturer::SourceList();
    if (_capturer && _capturer->GetSourceList(&list))
    {
        for (const auto &source : list)
        {
            printf("%s\n", to_string(source.id).c_str());
            printf("%s\n", source.title.c_str());
            myfile << to_string(source.id).c_str() << endl;
            myfile << source.title << endl;
        }
    }
    myfile.close();
    _capturer->SelectSource(0);
    _capturer->Start(&_callback);
    //_callback.setOutput(std::make_shared<webrtc::Sink>(false))
    _scheduler = CaptureScheduler();
    loop();
}

void DiscordPawgers::loop()
{
    printf("a");
    _capturer->CaptureFrame();
    const auto guard = std::weak_ptr<bool>(_timerGuard);
    _scheduler.runDelayed(1000. / 30, [=] {
        printf("b");
        if (guard.lock()) {
            printf("c");
            loop();
        } 
    });
}
