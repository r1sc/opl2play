#pragma once

#include <vector>
#include <deque>

class WaveoutDevice {
	HWAVEOUT waveout;

	typedef std::vector<int16_t> Buffer;

	std::vector<WAVEHDR> audio_headers;
	std::vector<Buffer> buffers;
	std::deque<size_t> queue;

	void push_free_buffer(size_t element_index);

public:
	BOOL shutting_down = FALSE;

	WaveoutDevice(size_t num_buffers, unsigned int sample_rate, unsigned int buffer_size);
	~WaveoutDevice();
	void onWaveOutProc(UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
	void queue_buffer();
	Buffer* get_current_buffer();
};