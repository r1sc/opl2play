#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "waveout.h"
//#include "opl3.h"
#include "emu8950.h"

int main() {
	unsigned int buffer_size = 8000;
	unsigned int sample_rate = 44100;

	waveout_initialize(sample_rate, buffer_size);

	/*opl3_chip chip;
	OPL3_Reset(&chip, sample_rate);*/

	OPL* opl = OPL_new();
	OPL_reset(opl);

	FILE* file = fopen("dream.vgm", "rb");

	uint32_t loop_offset = 0;
	fseek(file, 0x1C, SEEK_SET);
	fread(&loop_offset, 4, 1, file);
	if (loop_offset != 0) {
		loop_offset += 0x1C;
	}

	uint32_t data_offset;
	fseek(file, 0x34, SEEK_SET);
	fread(&data_offset, 4, 1, file);
	fseek(file, data_offset - 4, SEEK_CUR);

	size_t wait = 0;

	while (1) {
		
		int16_t* buffer = waveout_get_current_buffer();
		if (buffer != NULL) {
			int16_t* buffer_end = buffer + buffer_size;
			while(buffer < buffer_end) {
				//OPL3_GenerateResampled(&chip, buffer);
				int16_t sample = OPL_calc(opl);
				*buffer++ = sample;
				//sample = OPL_calc(opl);

				if (wait == 0) {
					uint8_t command;
					fread(&command, 1, 1, file);

					if (command == 0x5A) {
						uint8_t address_value[2];
						fread(address_value, 1, 2, file);
						//OPL3_WriteRegBuffered(&chip, address_value[0], address_value[1]);
						OPL_writeReg(opl, (uint32_t)address_value[0], address_value[1]);
					}
					else if (command == 0x5E) {
						uint8_t address_value[2];
						fread(address_value, 1, 2, file);
						//OPL3_WriteRegBuffered(&chip, address_value[0], address_value[1]);
					}
					else if (command == 0x5F) {
						uint8_t address_value[2];
						fread(address_value, 1, 2, file);
						//OPL3_WriteRegBuffered(&chip, address_value[0] | 0x100, address_value[1]);
					}
					else if (command == 0x61) {
						uint16_t samples;
						fread(&samples, 2, 1, file);
						wait += samples;
					}
					else if (command == 0x62) {
						wait += 735;
					}
					else if (command == 0x63) {
						wait += 882;
					}
					else if (command == 0x66) {
						if (loop_offset != 0) {
							fseek(file, loop_offset, SEEK_SET);
						}
						else {
							goto done;
						}
					}
					else if ((command & 0x70) == 0x70) {
						uint8_t num_waits = (wait & 0x0F) + 1;
						wait += num_waits;
					}
				}
				else {
					wait--;
				}
			}
			
			waveout_queue_buffer();
		}
		Sleep(16);
	}

	done:

	fclose(file);
	waveout_free();

	return 0;
}