// ------------------------------------------------------------------------
// Nunchuck / joystick handling
//
// Handles wii nunchuck on I2C bus
// Wraps communication and creates centered X/Y values
// Creates an 'events' system where for example pressing up
// latches the up press event until read out
// some code borrowed from http://www.windmeadow.com/node/42
// ------------------------------------------------------------------------
#include "joystickcontrol.h"

// initialize the I2C system, join the I2C bus,
// and tell the nunchuck we're talking to it
joystick_c::joystick_c() {
        Wire.begin();
        startup();
}

void joystick_c::startup() {
	#ifdef debugOut_I2CTimeouts
		Serial.println("Starting nunchuck comms");
	#endif
	connected = false;
	Wire.beginTransmission(NUNCHUCK_ADDRESS);// transmit to device
	Wire.write((uint8_t)0x40);// sends memory address
	Wire.write((uint8_t)0x00);// sends sent a zero.
	if(Wire.endTransmission()!=0) {
		#ifdef debugOut_I2CTimeouts
			Serial.println("Nunchuck send init fail");
		#endif
		return;
	} else {
		// Sent nunchuck init
		#ifdef debugOut_I2CTimeouts
			Serial.println("Nunchuck send init OK");
		#endif
	}
	bytestoingore = 20;
	digitalJoystick_Events = 0;
	newDataAvailibleEvent = false;
	lastCommunication = 0;
	return;
}

// Encode data to format that most wiimote drivers except
// only needed if you use one of the regular wiimote drivers
char joystick_c::nunchuk_decode_byte (char x) {
	x = (x ^ 0x17) + 0x17;
	return x;
}

void joystick_c::update() {
	// send a request for new nunchuck data in periods no shorter than 100ms
	static unsigned long lastupdate=millis();

	if(millis()<lastupdate+200) {
		return;    // only update only every 100ms+time taken to get here after
	}

	// get data (if availible)
	Wire.requestFrom(NUNCHUCK_ADDRESS, 6);
	unsigned long I2Ctimeout=millis()+50; // set 50ms timeout
	// wait until data availible or timeout
	while((Wire.available() < 6) && (millis() < I2Ctimeout)) { }
	if(Wire.available() == 6) { // ready to go
		nunchuck_joyx = Wire.read();  // byte 0, joyx
		nunchuck_joyy = Wire.read();  // byte 1, joyy
		nunchuck_accelx = Wire.read();  // byte 2, accelx
		nunchuck_accely = Wire.read();  // byte 3, accely
		nunchuck_accelz = Wire.read();  // byte 4, accelz
		byte buttonsByte = Wire.read(); // byte 5, buttonsbyte
		nunchuck_zbutton = ((nunchuk_decode_byte(buttonsByte) >> 0) & 1) ? 0 : 1;
		nunchuck_cbutton = ((nunchuk_decode_byte(buttonsByte) >> 1) & 1) ? 0 : 1;
		// create a 'centered' X, Y value pair from -128 to +128 with dead zone
		// ensure filtered x,y results are not generated in first 20 results as get randoms in this phase
		if(bytestoingore>0) {
			bytestoingore--;
			filteredX=0;
			filteredY=0;
		} else { // we are out of the first 20 weird bytes so generate results
			filteredX=nunchuck_joyx-124; // todo recal this at startup 22-124-228
			filteredY=nunchuck_joyy-132; // todo recal this at startup 227-131-41
			// filter dead zone
			if(filteredX > -20 && filteredX < 20) {
				filteredX=0;
			}
			if(filteredY > -20 && filteredY < 20) {
				filteredY=0;
			}
			// filter >100
			if(filteredX > 100) {
				filteredX=100;
			}
			if(filteredX < -100) {
				filteredX=-100;
			}
			if(filteredY > 100) {
				filteredY=100;
			}
			if(filteredY < -100) {
				filteredY=-100;
			}
			// update virtual DPAD where analog direction is convered into up down left right events
			updateDPAD();
			// flag to external object that data is availible
			newDataAvailibleEvent = true;
			lastCommunication = millis();
		}
	} else { // didnt get 6 bytes for some reason
		#ifdef debugOut_I2CTimeouts
			Serial.print("Nunchuck read data fail");
		#endif
		startup();
	}

	// request next dataload for next loop
	Wire.beginTransmission(NUNCHUCK_ADDRESS);// transmit to device
	Wire.write((uint8_t)0x00);// sends one byte
	if(Wire.endTransmission()==0) {
		// sucess
	} else {
		// fail TODO put in code to try restart nunchuck and skip expecting data next loop
		#ifdef debugOut_I2CTimeouts
			Serial.println("Nunchuck no ACK");
		#endif
	}

}

// maintains an event byte, bits from right to left are right, left, down, up, z, c, z+c long press
void joystick_c::updateDPAD() {
	static byte previousStates;
	byte currentStates = 0;

	// HANDLE DPAD Events
	// Convert analog values to on off bits
	currentStates = currentStates | ((bool)(filteredY > 70) << 3); // up
	currentStates = currentStates | ((bool)(filteredY < -70) << 2); // down
	currentStates = currentStates | ((bool)(filteredX < -70) << 1); // left
	currentStates = currentStates | ((bool)(filteredX > 70) << 0); // right
	currentStates = currentStates | (((bool)nunchuck_zbutton & 1) << 4); // z
	currentStates = currentStates | (((bool)nunchuck_cbutton & 1) << 5); // c
	currentStates = currentStates | (( (bool)nunchuck_zbutton && (bool)nunchuck_cbutton ) << 6);

	// create gone high bits + xor the current bits with the old bits
	// and then & them to current bits to only be 1 if now high and was low
	byte digitalJoystick_goneHigh = (currentStates ^ previousStates) & currentStates;

	// set event bits high for gone high directions
	digitalJoystick_Events = digitalJoystick_Events | digitalJoystick_goneHigh;

	// store states for next comparision
	previousStates = currentStates;
	digitalJoystick_prevstates = currentStates;
}

// functions for other objects to check for an event
bool joystick_c::up_event() {
	return readDPADValueAndClearEvent(3);
}

bool joystick_c::down_event() {
	return readDPADValueAndClearEvent(2);
}

bool joystick_c::left_event() {
	return readDPADValueAndClearEvent(1);
}

bool joystick_c::right_event() {
	return readDPADValueAndClearEvent(0);
}

bool joystick_c::zc_longpress_event() {
	return readDPADValueAndClearEvent(6);
}

bool joystick_c::newDataAvailible_event() {
	bool returnValue = newDataAvailibleEvent;
	newDataAvailibleEvent = false;
	return returnValue;
}

bool joystick_c::CommunicationTimeout() {
	return (millis() > (lastCommunication + 1000));
}

bool joystick_c::readDPADValueAndClearEvent(byte bittoread) {
	bool returnValue;
	returnValue = (digitalJoystick_Events >> bittoread) & 1; // get event bit
	digitalJoystick_Events = digitalJoystick_Events & (~( 1 << bittoread )); // unset event
	return returnValue;
}
