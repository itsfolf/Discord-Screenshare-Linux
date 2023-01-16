#include "PipeWire.h"

#include "rtc_base/logging.h"
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>
#include <spa/debug/format.h>
#include <spa/pod/builder.h>
#include "out/Release/gen/modules/desktop_capture/linux/wayland/pipewire_stubs.h"
using modules_desktop_capture_linux_wayland::InitializeStubs;
using modules_desktop_capture_linux_wayland::kModuleDrm;
using modules_desktop_capture_linux_wayland::kModulePipewire;
using modules_desktop_capture_linux_wayland::StubPathMap;
const char kPipeWireLib[] = "libpipewire-0.3.so.0";
const char kDrmLib[] = "libdrm.so.2";
#include <iomanip>
#include <algorithm>
#include <spa/utils/ringbuffer.h>
#include "threads.h"

PipeWireEngine::PipeWireEngine(const char *category, void *param, const std::function<void(void *param)> callback)
{
    StubPathMap paths;

    // Check if the PipeWire and DRM libraries are available.
    paths[kModulePipewire].push_back(kPipeWireLib);
    paths[kModuleDrm].push_back(kDrmLib);

    if (!InitializeStubs(paths))
    {
        RTC_LOG(LS_ERROR)
            << "One of following libraries is missing on your system:\n"
            << " - PipeWire (" << kPipeWireLib << ")\n"
            << " - drm (" << kDrmLib << ")";
        return;
    }

    pw_init(nullptr, nullptr);
    m_loop = pw_loop_new(nullptr);
    if (!m_loop)
    {
        return;
    }

    pw_properties *props =
        pw_properties_new(PW_KEY_APP_NAME, "Discord Linux Fix", PW_KEY_NODE_NAME, category, PW_KEY_MEDIA_CATEGORY, category,
                          PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_ROLE, "Production", nullptr);
    m_events = std::make_unique<pw_stream_events>();
    m_events->version = PW_VERSION_STREAM_EVENTS;
    m_events->process = *callback.target<void (*)(void *)>();
    m_events->param_changed = [](void *data, uint32_t id, const struct spa_pod *param)
    {
        if (param == nullptr || id != SPA_PARAM_Format)
        {
            return;
        }

        struct spa_audio_info_raw info;
        if (spa_format_audio_raw_parse(param, &info) < 0)
        {
            return;
        }

        RTC_LOG(LS_INFO) << "PipeWireEngine::param_changed: " << info.rate << "Hz, " << info.channels << " channels "
                         << " format: " << info.format;

        switch (info.format)
        {
        case SPA_AUDIO_FORMAT_U8:
            RTC_LOG(LS_INFO) << "PipeWireEngine::param_changed: format: SPA_AUDIO_FORMAT_U8";
            break;
        case SPA_AUDIO_FORMAT_S16_LE:
            RTC_LOG(LS_INFO) << "PipeWireEngine::param_changed: format: SPA_AUDIO_FORMAT_S16_LE";
            break;
        case SPA_AUDIO_FORMAT_S32_LE:
            RTC_LOG(LS_INFO) << "PipeWireEngine::param_changed: format: SPA_AUDIO_FORMAT_S32_LE";
            break;
        case SPA_AUDIO_FORMAT_F32_LE:
            RTC_LOG(LS_INFO) << "PipeWireEngine::param_changed: format: SPA_AUDIO_FORMAT_F32_LE";
            break;
        default:
            RTC_LOG(LS_INFO) << "PipeWireEngine::param_changed: format: unknown";
            break;
        }
    };
    m_stream = pw_stream_new_simple(m_loop, category, props, m_events.get(), param);
    if (!m_stream)
    {
        return;
    }

    m_thread = pw_thread_loop_new_full(m_loop, category, nullptr);
    if (m_thread)
    {
        m_ok = true;
    }
};

PipeWireEngine::~PipeWireEngine()
{
    if (m_thread)
    {
        pw_thread_loop_destroy(m_thread);
    }

    if (m_stream)
    {
        pw_stream_destroy(m_stream);
    }

    if (m_loop)
    {
        pw_loop_destroy(m_loop);
    }
}

bool PipeWireEngine::connect(const uint8_t direction, const uint8_t nChannels)
{
    uint8_t buffer[2048];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    const struct spa_pod *params[2];
    uint32_t *position;
    position[0] = SPA_AUDIO_CHANNEL_MONO;
    params[0] = (spa_pod *)spa_pod_builder_add_object(&b,
                                                      SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
                                                      //  specify the media type and subtype
                                                      SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_audio),
                                                      SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
                                                      SPA_FORMAT_AUDIO_format, SPA_POD_CHOICE_ENUM_Id(4, SPA_AUDIO_FORMAT_U8, SPA_AUDIO_FORMAT_S16_LE, SPA_AUDIO_FORMAT_S32_LE, SPA_AUDIO_FORMAT_F32_LE),
                                                      SPA_FORMAT_AUDIO_channels, SPA_POD_Int(1),
                                                      SPA_FORMAT_AUDIO_position, SPA_POD_Array(sizeof(uint32_t), SPA_TYPE_Id, 1, position),
                                                      SPA_FORMAT_AUDIO_rate, SPA_POD_Int(48000));
    /* params[1] = (spa_pod *)spa_pod_builder_add_object(&b,
         SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
         SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(8, 2, 64),
         SPA_PARAM_BUFFERS_blocks,  SPA_POD_Int(1),
         SPA_PARAM_BUFFERS_size,    SPA_POD_Int(441 * 4),
         SPA_PARAM_BUFFERS_stride,  SPA_POD_Int(1),
         SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int((1<<SPA_DATA_MemPtr)));*/

    // params[1] = spa_latency_build(&b, SPA_PARAM_Latency, 10);

    return pw_stream_connect(m_stream, (spa_direction)direction, PW_ID_ANY,
                             (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
                             params, 1) == 0;
}

void PipeWireEngine::start()
{
    if (m_thread)
    {
        pw_thread_loop_start(m_thread);
    }
}

void PipeWireEngine::stop()
{
    if (m_loop)
    {
        pw_thread_loop_stop(m_thread);
    }
}

pw_buffer *PipeWireEngine::dequeueBuffer()
{
    if (m_stream)
    {
        return pw_stream_dequeue_buffer(m_stream);
    }

    return nullptr;
}

void PipeWireEngine::queueBuffer(pw_buffer *buffer)
{
    if (m_stream)
    {
        pw_stream_queue_buffer(m_stream, buffer);
    }
}

PipeWireInput::PipeWireInput(void *callback_)
{
    callback = callback_;
    m_engine = std::make_unique<PipeWireEngine>("Capture", this, processCallback);
    if (!m_engine->isOk())
    {
        return;
    }

    if (!m_engine->connect(PW_DIRECTION_INPUT, 2))
    {
        return;
    }

    LinuxFix::GlobalCaptureThread()->PostTask([this]()
                                           { run(); });
    m_engine->start();
}

PipeWireInput::~PipeWireInput()
{
    m_engine->stop();
}

void PipeWireInput::processCallback(void *param)
{
    auto pwi = static_cast<PipeWireInput *>(param);

    pw_buffer *buffer = pwi->m_engine->dequeueBuffer();
    if (!buffer)
    {
        return;
    }

    spa_data &data = buffer->buffer->datas[0];
    if (!data.data)
    {
        return;
    }
    RTC_LOG(LS_INFO) << buffer->size;

    if (data.type != SPA_DATA_MemPtr)
    {
        RTC_LOG(LS_ERROR) << "PipeWireInput::processCallback: buffer type is not SPA_DATA_MemPtr" << data.type;
    }
    // pwi->addMic(data.data, data.chunk->size / sizeof(float));
    if (pwi->callback)
    {
        // uint frameSize = 480 * sizeof(float);
        if (!pwi->m_buffer)
        {
            pwi->m_buffer = (char *)malloc(data.maxsize);
            RTC_LOG(LS_INFO) << "PipeWireInput::processCallback: malloc " << pwi->m_buffer << " " << data.maxsize;
            pwi->m_bufferSize = 0;
            // pwi->out_buffer = malloc(frameSize);
            // spa_ringbuffer_init(&pwi->out_ring);
        }
        // // append all data to pwi->m_buffer, without replacing it
        // //RTC_LOG(LS_INFO) << "PipeWireInput::processCallback: new frames " << (data.chunk->size / sizeof(float));
        // float out_buf[frameSize / sizeof(float)];
        // uint32_t pindex;
        // spa_ringbuffer_get_read_index(&pwi->out_ring, &pindex);

        // spa_ringbuffer_read_data(&pwi->out_ring, pwi->out_buffer,
        // 		pwi->out_ringsize, pindex % pwi->out_ringsize,
        // 		(void *)out_buf, frameSize);
        // ((void (*)(void *, int64_t, int64_t, int64_t, uint))pwi->callback)(out_buf, 480, 4, 1, 48000);

        RTC_LOG(LS_INFO) << "PipeWireInput::processCallback: new frames " << data.chunk->size;
        memcpy((char *)pwi->m_buffer + pwi->m_bufferSize, data.data, data.chunk->size);
        pwi->m_bufferSize += data.chunk->size;

        // while (pwi->m_bufferSize >= 480 * sizeof(float))
        // {
        //     RTC_LOG(LS_INFO) << "PipeWireInput::processCallback: consuming " << pwi->m_bufferSize << "-" << (480 * sizeof(float));
        //     //RTC_LOG(LS_INFO) << "PipeWireInput::processCallback: calling callback " << pwi->m_bufferSize;

        //     pwi->m_bufferSize -= 480 * sizeof(float);
        //     //RTC_LOG(LS_INFO) << "PipeWireInput::processCallback: calling callback " << pwi->m_bufferSize;
        //     memmove(pwi->m_buffer, (char *)pwi->m_buffer + 480 * sizeof(float), pwi->m_bufferSize);
        // }
        // memmove(data.data, ((char*)data.data) + std::max(0, (int)(data.chunk->size - 1764)), 1764);
        // RTC_LOG(LS_INFO) << "PipeWireInput::processCallback: new frames " << (data.chunk->size / sizeof(float)) << " offset: " << data.chunk->offset << " stride: " << data.chunk->stride << " mapoffset: " << data.mapoffset << " fd: " << data.fd << " maxsize: " << data.maxsize;
        // ((void (*)(void *, int64_t, int64_t, int64_t, uint))pwi->callback)(data.data, data.chunk->size / sizeof(float), 4, 1, data.chunk->size / sizeof(float) * 100);

        // pwi->m_bufferSize -= 3528 / 2;
        // memmove(pwi->m_buffer, (char *)pwi->m_buffer + (3528 /2), pwi->m_bufferSize);
        // RTC_LOG(LS_INFO) << "PipeWireInput::processCallback: frames remaining " << (pwi->m_bufferSize / sizeof(float));

        // RTC_LOG(LS_INFO) << "Calling callback chunkSize=" << data.chunk->size << " frames=" << data.chunk->size / sizeof(float);
    }

    pwi->m_engine->queueBuffer(buffer);
}

void PipeWireInput::run()
{
    RTC_LOG(LS_INFO) << "PipeWireInput::run";
    LinuxFix::GlobalCaptureThread()->PostDelayedTask([this]()
                                                  { run(); },
                                                  webrtc::TimeDelta::Millis(10));
    if (!m_engine->isOk() || !m_buffer || !callback)
    {
        RTC_LOG(LS_INFO) << "PipeWireInput::run: not ok";
        return;
    }
    while (m_bufferSize >= 480 * sizeof(float))
    {
        RTC_LOG(LS_INFO) << "PipeWireInput::run: consuming " << m_bufferSize << "-" << (480 * sizeof(float));
        // RTC_LOG(LS_INFO) << "PipeWireInput::processCallback: calling callback " << pwi->m_bufferSize;
        void* out_buf = malloc(480 * sizeof(float));
        memcpy(out_buf, m_buffer, 480 * sizeof(float));
        ((void (*)(void *, int64_t, int64_t, int64_t, uint))callback)(out_buf, 480, 4, 1, 48000);
        free(out_buf);

        m_bufferSize -= 480 * sizeof(float);
        // RTC_LOG(LS_INFO) << "PipeWireInput::processCallback: calling callback " << pwi->m_bufferSize;
        memmove(m_buffer, (char *)m_buffer + 480 * sizeof(float), m_bufferSize);
    }
    //RTC_LOG(LS_INFO) << "PipeWireInput::processCallback: new frames " << (data.chunk->size / sizeof(float)) << " offset: " << data.chunk->offset << " stride: " << data.chunk->stride << " mapoffset: " << data.mapoffset << " fd: " << data.fd << " maxsize: " << data.maxsize;
    //((void (*)(void *, int64_t, int64_t, int64_t, uint))pwi->callback)(data.data, data.chunk->size / sizeof(float), 4, 1, data.chunk->size / sizeof(float) * 100);
}
