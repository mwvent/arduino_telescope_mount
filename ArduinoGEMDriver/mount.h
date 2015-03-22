// ------------------------------------------------------------------------
// Telescope Mount Class
// A telescope mount is an RA and DEC axis class
// This class provides methods that need to work on both axis
// ------------------------------------------------------------------------
#ifndef _mount_h
#define _mount_h
#include "settings.h"
#include "axis.h"
#include "encoders.h"

class mount_c
{
	public:
		mount_c(uint8_t RAmotorPort, uint8_t DECmotorPort,
				int RAGearTeeth, int DECGearTeeth,
				int RAEncoderPPR, int DECEncoderPPR,  
				DS1307_RTC_c* DS1307_RTC_Obj,	EEPROMHandler_c* EEPROMObj);
		
	// Objects
	// -------
	public:
		ra_axis_c* ra_axis;
		dec_axis_c* dec_axis;
		DS1307_RTC_c* DS1307_RTC;
		EEPROMHandler_c* EEPROMHandler;
		
	// Polling
	// -------
	public:
		void update();

		
	// Control
	// -------
	public:
		void slewNorth(byte speed);
		void slewSouth(byte speed);
		void slewEast(byte speed);
		void slewWest(byte speed);
		void stopSlew();
		void joystickControl(int x, int y, bool fast);
		void moveTarget(int RAOffset, int DECOffset);
		void sync();
		void sync(position syncPos);
		void gotoPos(position gotoPos);
		
		
	// PEC Train
	// ---------
	public:
		unsigned int PECTrainRemainingSeconds();
		unsigned int PECTrainElapsedSeconds();
		void PECTrain(bool newState);
		void setPECTrainOffset(int newRAOffset, int newDECOffSet);
		bool PECTrainComplete();
	private:
		void PECTrainUpdate();
		bool PECTrainingOn;
		unsigned long PECTrainStartTime;
		int PECTrainRAOffSet;
		int PECTrainDECOffSet;
		unsigned long lastPECUpdate;
};

#endif
