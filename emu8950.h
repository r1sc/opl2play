#ifndef _EMU8950_H_
#define _EMU8950_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


	/* voice data */
	typedef struct __OPL_PATCH {
		uint8_t TL, FB, EG, ML, AR, DR, SL, RR, KR, KL, AM, PM, WS;
	} OPL_PATCH;

	/* mask */
#define OPL_MASK_CH(x) (1 << (x))
#define OPL_MASK_HH (1 << 9)
#define OPL_MASK_CYM (1 << 10)
#define OPL_MASK_TOM (1 << 11)
#define OPL_MASK_SD (1 << 12)
#define OPL_MASK_BD (1 << 13)
#define OPL_MASK_ADPCM (1 << 14)
#define OPL_MASK_RHYTHM (OPL_MASK_HH | OPL_MASK_CYM | OPL_MASK_TOM | OPL_MASK_SD | OPL_MASK_BD)

	/* slot */
	typedef struct __OPL_SLOT {
		uint8_t number;

		/* type flags:
		 * 000000SM
		 *       |+-- M: 0:modulator 1:carrier
		 *       +--- S: 0:normal 1:single slot mode (sd, tom, hh or cym)
		 */
		uint8_t type;

		OPL_PATCH __patch;
		OPL_PATCH* patch;  /* = alias for __patch */

		/* slot output */
		int32_t output[2]; /* output value, latest and previous. */

		/* phase generator (pg) */
		uint16_t* wave_table; /* wave table */
		uint32_t pg_phase;    /* pg phase */
		uint32_t pg_out;      /* pg output, as index of wave table */
		uint8_t pg_keep;      /* if 1, pg_phase is preserved when key-on */
		uint16_t blk_fnum;    /* (block << 9) | f-number */
		uint16_t fnum;        /* f-number (9 bits) */
		uint8_t blk;          /* block (3 bits) */

		/* envelope generator (eg) */
		uint8_t eg_state;         /* current state */
		uint16_t tll;             /* total level + key scale level*/
		uint8_t rks;              /* key scale offset (rks) for eg speed */
		uint8_t eg_rate_h;        /* eg speed rate high 4bits */
		uint8_t eg_rate_l;        /* eg speed rate low 2bits */
		uint32_t eg_shift;        /* shift for eg global counter, controls envelope speed */
		int16_t eg_out;           /* eg output */

		uint32_t update_requests; /* flags to debounce update */
	} OPL_SLOT;

	typedef struct __OPL {
		uint8_t notesel;

		uint8_t reg[0x100];
		uint8_t test_flag;
		uint32_t slot_key_status;
		uint8_t rhythm_mode;

		uint32_t eg_counter;

		uint32_t pm_phase;
		uint32_t pm_dphase;

		int32_t am_phase;
		int32_t am_dphase;
		uint8_t lfo_am;

		uint32_t noise;
		uint8_t short_noise;

		OPL_SLOT slot[18];
		uint8_t ch_alg[9]; // alg for each channels

		uint32_t mask;
		uint8_t am_mode;
		uint8_t pm_mode;

		/* channel output */
		/* 0..8:tone 9:bd 10:hh 11:sd 12:tom 13:cym 14:adpcm */
		int16_t ch_out[15];

		int16_t mix_out[2];

	} OPL;

	void OPL_reset(OPL*);
	void OPL_writeReg(OPL* opl, uint32_t reg, uint8_t val);
	int16_t OPL_calc(OPL* opl);	

#ifdef __cplusplus
}
#endif

#endif
