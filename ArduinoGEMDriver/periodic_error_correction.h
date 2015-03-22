// ------------------------------------------------------------------------
// Works with the ra axis class & the EEPROM storage class 
// to provide and set error for points on the RA axis relative to the
// index point (requires an indexed encoder to work)
// ------------------------------------------------------------------------
#ifndef _periodic_error_correction_h
#define _periodic_error_correction_h
#include "settings.h"
#include "mount.h"
#include "axis.h"
#include "EEPROMHandler.h"
class periodic_error_correction_c
{
	public:
		EEPROMHandler_c* EEPROM;
		int RAEncoderPPR;
		byte PECResolution;
		ra_axis_c* ra_axis;
		periodic_error_correction_c(int RAEncoderPPR, EEPROMHandler_c* EEPROMObj, mount_c* mount_obj);
		update();
}
#endif