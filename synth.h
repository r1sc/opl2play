#pragma once

#include <stdint.h>

#define NUM_CHANNELS 4

void synth_set_frequency(unsigned int channel_no, unsigned int frequency);
void synth_tick();