#include <Windows.h>
#include <cstdint>
#include <stdio.h>
#include <iostream>

#include "waveout.h"
#include "emu8950.h"

BOOL running = TRUE;

BOOL WINAPI consoleHandler(DWORD signal) {
	if (signal == CTRL_C_EVENT)
		running = FALSE; // do cleanup

	return TRUE;
}

int main(const int argc, const char** argv) {
	if (argc != 2) {
		std::cout << "usage: opl2play.exe <path-to-vgm-file>" << std::endl;
		return 1;
	}

	if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
		printf("\nERROR: Could not set control handler");
		return 1;
	}	

	FILE* file;
	auto err = fopen_s(&file, argv[1], "rb");
	if (err) {
		printf("Error: Failed to open file '%s'\n", argv[1]);
		return 1;
	}

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
	printf("%s - %s by %s (%s)\n(CTRL-C to stop)\n", gd3_parts[2], gd3_parts[0], gd3_parts[6], gd3_parts[8]);
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

	WaveoutDevice waveout(2, 49716, 8192);

	OPL opl;
	OPL_reset(&opl);

	unsigned int wait = 0;

	while (running) {		
		auto b = waveout.get_current_buffer();
		if (b != nullptr) {
			auto buffer = b->begin();
			while(buffer != b->end()) {
				int16_t sample = OPL_calc(&opl);
				*buffer++ = sample;				

				if (wait == 0) {
					uint8_t command;
					fread(&command, 1, 1, file);

					if (command == 0x5A) {
						uint8_t address_value[2];
						fread(address_value, 1, 2, file);
						OPL_writeReg(&opl, (uint32_t)address_value[0], address_value[1]);
					}
					else if (command == 0x61) {
						uint16_t samples;
						fread(&samples, 2, 1, file);
						wait += (unsigned int)(samples * 1.13);
					}
					else if (command == 0x62) {
						wait += (unsigned int)(735 * 1.13);
					}
					else if (command == 0x63) {
						wait += (unsigned int)(882 * 1.13);
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
						wait += (unsigned int)((unsigned int)num_waits * 1.13);
					}
				}
				else {
					wait--;
				}
			}
			
			waveout.queue_buffer();
		}
		Sleep(16);
	}

	done:

	fclose(file);

	return 0;
}