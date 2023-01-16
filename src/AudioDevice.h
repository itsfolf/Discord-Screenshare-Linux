

namespace LinuxFix
{
    class AudioDeviceBridge
    {
    public:
        static void SetAudioTransport(void *instance, char *transport);
        static void Attach(void *instance, uint deviceId);
        static void Detach();
        static bool IsSpeaking();
        static void AudioFrameCallback(void *audioSamples, int64_t samplesPerChannel, int64_t _bytesPerSample, int64_t numChannels, uint sampleRateHz);
        static uint (*RecordedDataIsAvailable)(void *engine, int streamType, void *audioSamples, int64_t samplesPerChannel, int64_t _bytesPerSample, int64_t numChannels, uint sampleRateHz, uint totalDelayMS, int clockDrift, uint currentMicLevel, bool keyPressed);
    private:
        
        static char *transport_;
    };
}