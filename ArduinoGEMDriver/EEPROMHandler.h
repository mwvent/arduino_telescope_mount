// ------------------------------------------------------------------------
// EEPROM Access Functions
// ------------------------------------------------------------------------
#ifndef _EEPROMHandler_h
#define _EEPROMHandler_h
#include "settings.h"
#ifdef _USE_DS1307_NVRAM
	#include "DS1307_RTC.h" 
#endif
#include <EEPROM.h>
#include <Arduino.h>

// Define EEPROM Layout
// Arduino Mega 4k=2000ints - DS1307=50 bytes of general p storage
#define cacheFirstNoOfBytes 10 // tells function how large the settings block on the EEPROM is
                               // so it can be cached to RAM
#define EEPos_Verscheck 1 // byte
#define EEPos_Verscheck_expected_data 101 // the EEPROM layout check value
#define EEPos_Flags1 2 // byte, From MSB-LSB UsePEC, InvertPEC, UseRADrift, InvertRADrift,
                       // UseDECDrift, InvertDECDrift - PEC/drift are not currentley used though
#define EEPos_RAMotorSlowSpeed 3 // byte
#define EEPos_DECMotorSlowSpeed 4 // byte
#define EEPos_RAErrorTolerance 5 // byte
#define EEPos_DECErrorTolerance 6 // byte

class EEPROMHandler_c
{
	public:
	        // value that is incremented every time a write is performed on cached settings area
		// allows object to check if EEPROM settigns area has been updated
		int EEPROMUpdates;
		#ifndef _USE_DS1307_NVRAM
			EEPROMHandler_c();
		#else
			EEPROMHandler_c(DS1307_RTC_c* iDS1307);
			DS1307_RTC_c* DS1307;
		#endif
		void update();
		byte readRASlowSpeed();
		byte readDECSlowSpeed();
		void setRASlowSpeed(byte newVal);
		void setDECSlowSpeed(byte newVal);
		byte readRAErrorTolerance();
		void setRAErrorTolerance(byte newVal);
		byte readDECErrorTolerance();
		void setDECErrorTolerance(byte newVal);
		// EEPos_Flags1
		boolean readUsePEC();
		boolean readInvertPEC();
		boolean readUseRADrift();
		boolean readInvertRADrift();
		boolean readUseDECDrift();
		boolean readInvertDECDrift();
		byte setBit(byte inByte, byte bitNo, boolean newVal);
		void setUsePEC(boolean newVal);
		void setInvertPEC(boolean newVal);
		void setUseRADrift(boolean newVal);
		void setInvertRADrift(boolean newVal);
		void setUseDECDrift(boolean newVal);
		void setInvertDECDrift(boolean newVal);
	private:
		// cache
		void setDefaults();
		byte cache[cacheFirstNoOfBytes];
		boolean writeData(unsigned int pos, int value);
		boolean writeData(int pos, int value) {
			return writeData((unsigned int)pos, value);
		};
		int readData(unsigned int pos);
		int readData(int pos) {
			return readData((unsigned int)pos);
		};
		boolean writeByte(unsigned int pos, byte value);
		byte readByte(unsigned int pos);
};

#endif