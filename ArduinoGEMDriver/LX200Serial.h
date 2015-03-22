#ifndef _LX200Serial_h_
#define _LX200Serial_h_
#include "settings.h"
#include <avr/pgmspace.h>
#include <arduino.h>
#include "EEPROMHandler.h"
#include "DS1307_RTC.h"
#include "mount.h"

class LX200SerialHandler_c
{
	public:
		LX200SerialHandler_c(mount_c* mountobj,  DS1307_RTC_c* DS1307_RTCObj, EEPROMHandler_c* EEPROMObj);
		void update();
	private:
		EEPROMHandler_c* EEPROMHandler;
		mount_c* mount;
		DS1307_RTC_c* DS1307_RTC;

		// flags and settable data - TODO move to EEPROM?
		bool twentyFour_hourmode;
		bool highPrecision;
		bool longFormat;
		position currentObjectPos;
		byte motionRate;

		// incoming buffer
		char serialBuffer[25];
		byte serialBuffer_count;

		// serial print and help functions
		void SerialSend(String sendText);
		void SerialWrite(byte sendByte);
		void printIntLeadingZero(int value);
		void printIntLeadingZeroAndPolarity(int value);
		int findCharacterAndChangeToStringTerminator(char* chars, int startPos, char charToFind);
		void progmemPrint (PGM_P s);
		void progmemPrintln (PGM_P s);

		// serial command processing
		void serialCommand();
		void printLX200positionRA(Angle* pos);
		void printLX200positionDEC(Angle* pos);
		bool setRA();
		bool setDEC();
		bool setLocalTime();
		bool setDate();

};

#endif

