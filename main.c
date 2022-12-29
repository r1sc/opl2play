#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "waveout.h"
#include "emu8950.h"
//#include "opl3.h"

BOOL running = TRUE;

BOOL WINAPI consoleHandler(DWORD signal) {
	if (signal == CTRL_C_EVENT)
		running = FALSE; // do cleanup

	return TRUE;
}

int main() {
	unsigned int buffer_size = 8000;
	unsigned int sample_rate = 49716;

	if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
		printf("\nERROR: Could not set control handler");
		return 1;
	}


	waveout_initialize(sample_rate, buffer_size);

	//opl3_chip chip;
	//OPL3_Reset(&chip, sample_rate);

	OPL opl;
	OPL_reset(&opl);

	FILE* file = fopen("dream.vgm", "rb");

	uint32_t gd3_offset = 0;
	fseek(file, 0x14, SEEK_SET);
	fread(&gd3_offset, 4, 1, file);
	fseek(file, gd3_offset, SEEK_CUR);

	fseek(file, 4, SEEK_CUR); // Skip version
	uint32_t gd3_size = 0;
	fread(&gd3_size, 4, 1, file);

	unsigned short* gd3_data = (unsigned short*)malloc(gd3_size);
	fread(gd3_data, gd3_size, 1, file);

	uint32_t gd3_size_half = gd3_size / 2;
	char* gd3_ascii = (char*)malloc(gd3_size_half);
	char* gd3_parts[12];
	gd3_parts[0] = gd3_ascii;

	uint8_t part_no = 1;
	for (size_t i = 0; i < gd3_size_half; i++) {
		unsigned short char_16 = gd3_data[i];
		gd3_ascii[i] = (unsigned char)char_16;
		if (char_16 == 0) {
			gd3_parts[part_no++] = gd3_ascii + i+1;
		}
	}
	printf("Playing %s - %s by %s (%s)\n(CTRL-C to stop)\n", gd3_parts[2], gd3_parts[0], gd3_parts[6], gd3_parts[8]);
	free(gd3_ascii);
	free(gd3_data);

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


	while (running) {		
		int16_t* buffer = waveout_get_current_buffer();
		if (buffer != NULL) {
			int16_t* buffer_end = buffer + buffer_size;
			while(buffer < buffer_end) {
				/*OPL3_GenerateResampled(&chip, buffer);
				buffer += 2;*/
				
				int16_t sample = OPL_calc(&opl);
				*buffer++ = sample;				

				if (wait == 0) {
					uint8_t command;
					fread(&command, 1, 1, file);

					if (command == 0x5A) {
						uint8_t address_value[2];
						fread(address_value, 1, 2, file);
						//OPL3_WriteRegBuffered(&chip, address_value[0], address_value[1]);
						OPL_writeReg(&opl, (uint32_t)address_value[0], address_value[1]);
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
						wait += samples * 1.13;
					}
					else if (command == 0x62) {
						wait += 735 * 1.13;
					}
					else if (command == 0x63) {
						wait += 882 * 1.13;
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
						wait += num_waits * 1.13;
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