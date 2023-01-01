#include "synth.h"
#include "waveout.h"
#include "strings.h"

const int sample_length = 6924;
const float sample_rate_hz = 22050.0f;
const float num_sine_curves_per_sec = 882;

float channel_pos[NUM_CHANNELS];
unsigned int channel_freq[NUM_CHANNELS];

void synth_set_frequency(unsigned int channel_no, unsigned int frequency) {
	channel_freq[channel_no] = frequency;
}

void synth_tick() {
	int16_t* buffer = waveout_get_current_buffer();
	if (buffer != NULL) {

		for (size_t i = 0; i < 2000; i++) {
			int16_t combined_samples = 0;

			for (size_t channel = 0; channel < NUM_CHANNELS; channel++)
			{
				unsigned int hz = channel_freq[channel];
				if (hz == 0) {
					combined_samples += 0;
				}
				else {
					float steps_per_sample = hz / num_sine_curves_per_sec;

					// float steps_per_sample = (float)(multiplier * state->hz) / sample_rate_hz;
					int16_t sample = (int16_t)strings[(int)channel_pos[channel]];

					channel_pos[channel] += steps_per_sample;
					if (channel_pos[channel] >= sample_length) {
						channel_pos[channel] -= sample_length;
					}
					combined_samples += sample;
				}
			}

			int16_t result_sample = combined_samples / NUM_CHANNELS;
			buffer[i] = result_sample;
		}

		waveout_queue_buffer();
	}
}