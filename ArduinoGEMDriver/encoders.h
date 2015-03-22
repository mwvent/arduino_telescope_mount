// ------------------------------------------------------------------------
// Encoder handling
// ------------------------------------------------------------------------
#ifndef _encoders_h
#define _encoders_h
#include "settings.h"
#include "DS1307_RTC.h"
#include "EEPROMHandler.h"
#include "axis.h"

class ra_axis_c;
class dec_axis_c;

class encoders_c {
	public:
		void begin(ra_axis_c* raObj, dec_axis_c* decObj);
		void interrupt();
		int readRAencoderMovement();
		int readDECencoderMovement();
		int readRAshaftMovement();
		int8_t readRAshaftReset();
	private:
		static ra_axis_c* raAxis;
		static dec_axis_c* decAxis;
		static volatile int DEC_movement;
		static volatile int RA_movement;
		// 0,none -1 backwards over Z, 1 forwards over Z
		static volatile int8_t RA_shaftreset;
		// may differ to RA_movement due to Z reset
		static volatile int RA_shaftmovement;
};

extern encoders_c encoders;

#endif
