// ------------------------------------------------------------------------
// EEPROM Access Functions
// ------------------------------------------------------------------------
#include "EEPROMHandler.h"

#ifndef _USE_DS1307_NVRAM
EEPROMHandler_c::EEPROMHandler_c() {
#else
EEPROMHandler_c::EEPROMHandler_c(DS1307_RTC_c* iDS1307) {
	DS1307 = iDS1307;
#endif
	#ifdef debugOut_startuplog
		Serial.println("    -> Reading settings from EEPROM");
		delay(200); // causes odd behaviour if there isnt one?
	#endif
	// read cached bytes from EEPROM to cache array
	#ifndef _USE_DS1307_NVRAM
		for(byte pos=0; pos<=cacheFirstNoOfBytes; pos++) {
			cache[pos] = EEPROM.read(pos);
			delay(100);
		}
	#else
		DS1307->blockRead(0, cacheFirstNoOfBytes, cache);
	#endif
	// check version byte - if it doesnt match then do default settings
	if(!(readByte(EEPos_Verscheck) == (byte)EEPos_Verscheck_expected_data)) {
		setDefaults();
	}
	// set inital value for EEPROMUpdates
	EEPROMUpdates = 1;
	delay(100);
}

void EEPROMHandler_c::setDefaults() {
	setRASlowSpeed((byte)_default_RASlowSpeed);
	setDECSlowSpeed((byte)_default_DECSlowSpeed);
	setRAErrorTolerance((byte)_default_RAErrorTolerance);
	setDECErrorTolerance((byte)_default_DECErrorTolerance);
	writeByte(EEPos_Verscheck, (byte)EEPos_Verscheck_expected_data);
}

signed int EEPROMHandler_c::readData(unsigned int pos)
{
	if((pos+1)<=cacheFirstNoOfBytes) {
		byte MSB = cache[(byte)pos];
		byte LSB = cache[(byte)(pos+1)];
		signed int result = (((signed int)MSB) << 8) | (signed int)LSB;
		return result;
	} else {
		// if _USE_DS1307_NVRAM - still use the EEPROM for anything outside settings memory
		byte MSB = EEPROM.read(pos);
		byte LSB = EEPROM.read(pos + 1);
		signed int result = (((signed int)MSB) << 8) | (signed int)LSB;
		return result;
	}
}

byte EEPROMHandler_c::readByte(unsigned int pos)
{
	if(pos<=cacheFirstNoOfBytes) {
		return cache[(byte)pos];
	}
	// if _USE_DS1307_NVRAM - still use the EEPROM for anything outside settings memory
	return (byte)EEPROM.read(pos);
}

boolean EEPROMHandler_c::writeData(unsigned int pos, signed int value)
{
	byte MSB = (byte)(value >> 8);
	byte LSB = (byte)(value & 0x00FF);
	if((pos+1)<=cacheFirstNoOfBytes) {
		#ifndef _USE_DS1307_NVRAM
			EEPROM.write(pos,MSB);
			EEPROM.write(pos + 1, LSB);
		#else
			DS1307->writeByte(pos, MSB);
			DS1307->writeByte(pos + 1, LSB);
		#endif
		cache[(byte)pos] = MSB;
		cache[(byte)(pos+1)] = LSB;
		EEPROMUpdates++;
	} else {
	        // if _USE_DS1307_NVRAM use the EEPROM outside the settings area anyway
		EEPROM.write(pos,MSB);
		EEPROM.write(pos + 1, LSB);
	}
	return true;
}

boolean EEPROMHandler_c::writeByte(unsigned int pos, byte value) {
	if(pos<=cacheFirstNoOfBytes) {
		#ifndef _USE_DS1307_NVRAM
			EEPROM.write(pos, value);
		#else
			DS1307->writeByte(pos, value);
		#endif
		cache[(byte)pos] = value;
		EEPROMUpdates++;
	} else {
// 		// if _USE_DS1307_NVRAM use the EEPROM anyway outside settings memory
		EEPROM.write(pos, value);
	}
	return true;
}

// -------------------------------------------------------------
// Access defined EEPROM values (abstract the EEPROM layout)
// -------------------------------------------------------------
byte EEPROMHandler_c::readRASlowSpeed() {
	return readByte(EEPos_RAMotorSlowSpeed);
}
byte EEPROMHandler_c::readDECSlowSpeed() {
	return readByte(EEPos_DECMotorSlowSpeed);
}
byte EEPROMHandler_c::readRAErrorTolerance() {
	return readByte(EEPos_RAErrorTolerance);
}
byte EEPROMHandler_c::readDECErrorTolerance() {
	return readByte(EEPos_DECErrorTolerance);
}
void EEPROMHandler_c::setRASlowSpeed(byte newVal) {
	writeByte(EEPos_RAMotorSlowSpeed, newVal);
}
void EEPROMHandler_c::setDECSlowSpeed(byte newVal) {
	writeByte(EEPos_DECMotorSlowSpeed, newVal);
}
void EEPROMHandler_c::setRAErrorTolerance(byte newVal) {
	writeByte(EEPos_RAErrorTolerance, newVal);
}
void EEPROMHandler_c::setDECErrorTolerance(byte newVal) {
	writeByte(EEPos_DECErrorTolerance, newVal);
}
// EEPos_Flags1
boolean EEPROMHandler_c::readUsePEC() {
	return (((readByte(EEPos_Flags1)>>7)&1) == 1);
}
boolean EEPROMHandler_c::readInvertPEC() {
	return (((readByte(EEPos_Flags1)>>6)&1) == 1);
}
boolean EEPROMHandler_c::readUseRADrift() {
	return (((readByte(EEPos_Flags1)>>5)&1) == 1);
}
boolean EEPROMHandler_c::readInvertRADrift() {
	return (((readByte(EEPos_Flags1)>>4)&1) == 1);
}
boolean EEPROMHandler_c::readUseDECDrift() {
	return (((readByte(EEPos_Flags1)>>3)&1) == 1);
}
boolean EEPROMHandler_c::readInvertDECDrift() {
	return (((readByte(EEPos_Flags1)>>2)&1) == 1);
}
byte EEPROMHandler_c::setBit(byte inByte, byte bitNo, boolean newVal) {
	if(newVal) {
		return inByte | (1 << bitNo);
	} else {
		return inByte & ~(1 << bitNo);
	}
}
void EEPROMHandler_c::setUsePEC(boolean newVal) {
	writeByte(EEPos_Flags1, setBit(readByte(EEPos_Flags1), 7, newVal));
}
void EEPROMHandler_c::setInvertPEC(boolean newVal) {
	writeByte(EEPos_Flags1, setBit(readByte(EEPos_Flags1), 6, newVal));
}
void EEPROMHandler_c::setUseRADrift(boolean newVal) {
	writeByte(EEPos_Flags1, setBit(readByte(EEPos_Flags1), 5, newVal));
}
void EEPROMHandler_c::setInvertRADrift(boolean newVal) {
	writeByte(EEPos_Flags1, setBit(readByte(EEPos_Flags1), 4, newVal));
}
void EEPROMHandler_c::setUseDECDrift(boolean newVal) {
	writeByte(EEPos_Flags1, setBit(readByte(EEPos_Flags1), 3, newVal));
}
void EEPROMHandler_c::setInvertDECDrift(boolean newVal) {
	writeByte(EEPos_Flags1, setBit(readByte(EEPos_Flags1), 2, newVal));
}


void EEPROMHandler_c::update() {
	// not used
}
