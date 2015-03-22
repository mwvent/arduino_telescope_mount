// -------------------------------------------------------------
// Telescope Driver
// -------------------------------------------------------------
#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include "settings.h"

// LCD & LCD I2C Libraries
// https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
// This replaces original Arduino LCD Library, remove the original
// library to avoid confilicts
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

// library for the adafruit motor sheild
// http://www.ladyada.net/make/mshield/
#include <AFMotor.h>
#include "DateTime.h"
#include "types.h"
#include "DS1307_RTC.h"
#include "Camera.h"
#include "EEPROMHandler.h"
#include "axis.h"
#include "mount.h"
#include "encoders.h"
#include "joystickcontrol.h"
#include "LX200Serial.h"
#include "UI.h"

DS1307_RTC_c* DS1307_RTC;
EEPROMHandler_c* EEPROMHandler;
mount_c* mount;
LX200SerialHandler_c* LX200SerialHandler;
UI_c* UI;

void setup() {
	#ifdef debugOut_startuplog
		delay(500);
	#endif
	Serial.begin(9600);
	#ifdef _LX200_TIMEOUT_FIX
		Serial.print("G#");
	#endif
	#ifdef debugOut_startuplog
		delay(500);
	#endif
        #ifdef debugOut_startuplog
		Serial.println("Starting up");
	#endif
	#ifdef debugOut_startuplog
		delay(500);
	#endif
	#ifdef debugOut_startuplog
		Serial.println("Starting wire");
	#endif
	Wire.begin(); // called in classes that use it but seems to hang if its not here
	#ifdef debugOut_startuplog
		Serial.println("Starting DS1307 RTC");
	#endif
	DS1307_RTC = new DS1307_RTC_c();
	#ifdef debugOut_startuplog
		Serial.println("Starting EEPROM handler");
	#endif
	#ifndef _USE_DS1307_NVRAM
		EEPROMHandler = new EEPROMHandler_c();
	#else
		EEPROMHandler = new EEPROMHandler_c(DS1307_RTC);
	#endif
	#ifdef debugOut_startuplog
		Serial.println("Starting Mount handler");
	#endif
	mount = new mount_c((uint8_t)_RA_MOTOR_PORT, (uint8_t)_DEC_MOTOR_PORT,
						_RA_ENCODER_GEAR_TEETH, _DEC_ENCODER_GEAR_TEETH,
						_RA_ENCODER_PPR, _DEC_ENCODER_PPR,
						DS1307_RTC, EEPROMHandler);
	#ifdef debugOut_startuplog
		Serial.println("Starting serial command parser");
	#endif
	LX200SerialHandler = new LX200SerialHandler_c(mount, DS1307_RTC, EEPROMHandler);
	#ifdef debugOut_startuplog
		Serial.println("Starting UI");
	#endif
	UI = new UI_c(mount, DS1307_RTC, EEPROMHandler);
	#ifdef debugOut_startuplog
		Serial.println("Startup complete");
	#endif
}

void loop() {
	LX200SerialHandler->update();
	mount->update();
	UI->update();
	mount->update();
	EEPROMHandler->update();
}
