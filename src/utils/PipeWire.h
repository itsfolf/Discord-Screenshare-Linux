#include <functional>
#include <pipewire/pipewire.h>
#include <pipewire/core.h>
#include <pipewire/keys.h>
#include <pipewire/stream.h>
#include <memory>
#include <spa/utils/ringbuffer.h>

class PipeWireEngine
{
public:
	bool isOk() { return m_ok; };

	bool connect(const uint8_t direction, const uint8_t nChannels);

	void start();
	void stop();

	pw_buffer *dequeueBuffer();
	void queueBuffer(pw_buffer *buffer);

	PipeWireEngine(const char *category, void *param, const std::function<void(void *param)> callback);
	~PipeWireEngine();

protected:
	bool m_ok;
	pw_loop *m_loop;
	pw_stream *m_stream;
	pw_thread_loop *m_thread;
	std::unique_ptr<pw_stream_events> m_events;
};

class PipeWireInput
{
public:
	void run();

	PipeWireInput(void *callback);
	~PipeWireInput();

protected:
	std::unique_ptr<PipeWireEngine> m_engine;
	void *callback = nullptr;
	char *m_buffer = nullptr;
	void *out_buffer;
	uint32_t out_ringsize;
	struct spa_ringbuffer out_ring;

	int m_bufferSize = 0;

	static void processCallback(void *param);
	void onFrame();
};