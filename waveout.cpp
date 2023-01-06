#include <Windows.h>
#include "waveout.h"

void CALLBACK waveOutProc(
	HWAVEOUT  hwo,
	UINT      uMsg,
	DWORD_PTR dwInstance,
	DWORD_PTR dwParam1,
	DWORD_PTR dwParam2
) {
	WaveoutDevice* waveoutDevice = (WaveoutDevice*)dwInstance;
	waveoutDevice->onWaveOutProc(uMsg, dwParam1, dwParam2);
}

WaveoutDevice::WaveoutDevice(size_t num_buffers, unsigned int sample_rate, unsigned int buffer_size) {
	WORD nChannels = 1;
	WORD wBitsPerSample = 16;
	WORD nBlockAlign = (nChannels * wBitsPerSample) / 8;
	DWORD nAvgBytesPerSec = sample_rate * nBlockAlign;

	WAVEFORMATEX format{
		.wFormatTag = WAVE_FORMAT_PCM,
		.nChannels = nChannels, // mono
		.nSamplesPerSec = sample_rate,
		.nAvgBytesPerSec = nAvgBytesPerSec,
		.nBlockAlign = nBlockAlign,
		.wBitsPerSample = wBitsPerSample,
		.cbSize = 0
	};

	MMRESULT result = waveOutOpen(&waveout, WAVE_MAPPER, &format, (DWORD_PTR)waveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
	if (result != MMSYSERR_NOERROR) {
		throw "Error opening device";
	}

	// Prepare buffers
	for (size_t i = 0; i < num_buffers; i++) {
		auto&& buffer = this->buffers.emplace_back(buffer_size);
		push_free_buffer(i);

		this->audio_headers.emplace_back(WAVEHDR{
			.lpData = (LPSTR)buffer.data(),
			.dwBufferLength = (DWORD)(buffer.size() * sizeof(int16_t)),
			.dwUser = (DWORD_PTR)i
			});
	}
}

WaveoutDevice::~WaveoutDevice() {
	shutting_down = TRUE;
	waveOutReset(waveout);
	for (auto&& audio_header : this->audio_headers) {
		waveOutUnprepareHeader(waveout, &audio_header, sizeof(WAVEHDR));
	}
	waveOutClose(waveout);
}

void WaveoutDevice::onWaveOutProc(UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMsg == WOM_DONE && !shutting_down) {
		LPWAVEHDR audio_header = (LPWAVEHDR)dwParam1;
		MMRESULT result = waveOutUnprepareHeader(waveout, audio_header, sizeof(WAVEHDR));
		if (result != MMSYSERR_NOERROR) {
			throw "Error unpreparing header: " + result;
		}

		push_free_buffer((size_t)audio_header->dwUser);
	}
}

void WaveoutDevice::push_free_buffer(size_t element_index) {
	this->queue.push_back(element_index);
}

void WaveoutDevice::queue_buffer()
{
	auto element_index = this->queue.front();
	this->queue.pop_front();

	WAVEHDR& audio_header = this->audio_headers[element_index];

	MMRESULT result = waveOutPrepareHeader(waveout, &audio_header, sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR) {
		throw "Error preparing header: " + result;
	}
	result = waveOutWrite(waveout, &audio_header, sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR) {
		throw "Error writing wave data: " + result;
	}
}

WaveoutDevice::Buffer* WaveoutDevice::get_current_buffer()
{
	if (this->queue.empty())
		return nullptr;

	auto element_index = this->queue.front();
	return &this->buffers[element_index];
}