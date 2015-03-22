// ------------------------------------------------------------------------
// Nunchuck / joystick handling
//
// Handles wii nunchuck on I2C bus
// Wraps communication and creates centered X/Y values
// Creates an 'events' system where for example pressing up
// latches the up press event until read out
// some code borrowed from http://www.windmeadow.com/node/42
// ------------------------------------------------------------------------
#ifndef _joystickcontol_h_
#define _joystickcontol_h_
#define NUNCHUCK_ADDRESS 0x52
#include "settings.h"
#include <Arduino.h>
#include <Wire.h>

class joystick_c
{
	public:
		joystick_c();
		void startup();
		void update();
		int nunchuck_zbutton;
		int nunchuck_cbutton;
		int nunchuck_joyx;
		int nunchuck_joyy;
		int nunchuck_accelx;
		int nunchuck_accely;
		int nunchuck_accelz;
		int filteredX;
		int filteredY;
		byte bytestoingore;
		// events and virtual DPAD
		bool up_event();
		bool down_event();
		bool left_event();
		bool right_event();
		bool newDataAvailible_event();
		bool CommunicationTimeout();
		bool zc_longpress_event();
		byte digitalJoystick_Events;
		// as above but bits get set high when currentstates go high and stay high until cleared
		byte digitalJoystick_prevstates;
	private:
		bool connected;
		void nunchuck_send_request();
		char nunchuk_decode_byte (char x);
		int nunchuck_get_data();
		// events and virtual DPAD
		void updateDPAD();
		bool readDPADValueAndClearEvent(byte bittoread);
		bool newDataAvailibleEvent;
		unsigned long lastCommunication;
};

#endif
