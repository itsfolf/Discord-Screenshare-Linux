#include "rtc_base/logging.h"

#include "AudioDevice.h"
#include "utils/PipeWire.h"

namespace LinuxFix
{
    char *AudioDeviceBridge::transport_;
    uint (*AudioDeviceBridge::RecordedDataIsAvailable)(void *engine, int streamType, void *audioSamples, int64_t samplesPerChannel, int64_t _bytesPerSample, int64_t numChannels, uint sampleRateHz, uint totalDelayMS, int clockDrift, uint currentMicLevel, bool keyPressed);
    bool done = false;

    void AudioDeviceBridge::SetAudioTransport(void *instance, char *transport)
    {
        RTC_LOG(LS_INFO) << "AudioDevice::SetAudioTransport";
        transport_ = transport;
        return;
    }

    void AudioDeviceBridge::Attach(void *instance, uint deviceId)
    {
        RTC_LOG(LS_INFO) << "AudioDevice::Attach, deviceId: " << deviceId;
        if (!done)
        {
            new PipeWireInput((void *)AudioFrameCallback);
            done = true;
        }
        // auto capture = pipewire_audio_capture_create(deviceId, (void*)AudioFrameCallback, PIPEWIRE_AUDIO_CAPTURE_OUTPUT);
        //  for (auto const &x : transport_->sendStreams_)
        //  {
        //      RTC_LOG(LS_INFO) << x.first << ": sampleRate=" << x.second.sampleRate << ", channelCount=" << x.second.channelCount << ", streamType=" << x.second.streamType;
        //  }
        return;
    }

    void AudioDeviceBridge::AudioFrameCallback(void *audioSamples, int64_t samplesPerChannel, int64_t _bytesPerSample, int64_t numChannels, uint sampleRateHz)
    {
        // RTC_LOG(LS_INFO) << "AudioDevice::AudioFrameCallback " << audioSamples << " " << (void*)RecordedDataIsAvailable;
        if (RecordedDataIsAvailable != nullptr)
        {
            uint res = RecordedDataIsAvailable(transport_, 1, audioSamples, samplesPerChannel, _bytesPerSample, numChannels, sampleRateHz, 0, 0, 0, false);
            if (res == -1)
            {
                RTC_LOG(LS_INFO) << "AudioDevice::AudioFrameCallback: RecordedDataIsAvailable returned -1";
            }
        }
    }

    bool AudioDeviceBridge::IsSpeaking() {
        return true;
    }

    void AudioDeviceBridge::Detach()
    {
        RTC_LOG(LS_INFO) << "AudioDevice::Detach";
        return;
    }
}