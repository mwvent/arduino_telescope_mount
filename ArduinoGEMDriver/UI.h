// ------------------------------------------------------------------------
// User Interface
// ------------------------------------------------------------------------
#ifndef _ui_h_
#define _ui_h_
#include "settings.h"
#include "mount.h"
#include "DS1307_RTC.h"
#include "EEPROMHandler.h"
#include "joystickcontrol.h"
#include "Camera.h"

// LCD & LCD I2C Libraries
// https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
// This replaces original Arduino LCD Library with must be removed first
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

// I2C LCD Display Library Setup
#define I2C_ADDR    0x3F  // Define I2C Address where the PCF8574A is
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

// UI Class - handle input from the joystick (wii nunchuck class) and output to the I2C LCD Display
class UI_c
{
	public:
		UI_c(mount_c* mount_obj, DS1307_RTC_c* DS1307_RTC_Obj, EEPROMHandler_c* EEPROM_obj);
		void update();
	private:
		// references
		LiquidCrystal_I2C* lcd;
		camera_c* camera;
		joystick_c* joystick;
		mount_c* mount;
		AF_DCMotor* focuser_motor;
		DS1307_RTC_c* DS1307_RTC;
		EEPROMHandler_c* EEPROMHandler;
		// helper functions
		void progmemLCDPrint (PGM_P s);
		void LCDPrintbool (bool val);
		void LCDPrintbyte (byte val);
		int freeRAM();
		// menu handler functions
		byte mainMenu(bool UIUpdateFlag);
		bool menuItem_TrackPan(bool UIUpdateFlag);
		bool menuItem_FineTrackPan(bool UIUpdateFlag);
		bool menuItem_Settings(bool UIUpdateFlag);
		bool menuItem_PECTrain(bool UIUpdateFlag);
		bool menuItem_Focuser(bool UIUpdateFlag);
		bool menuItem_ShaftPos(bool UIUpdateFlag);
};

#endif
