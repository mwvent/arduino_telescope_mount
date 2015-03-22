// ----------------------------------------------------------------------------------------------------------------
// Control camera
// -----------------------------------------------------------------------------------------------------------------
#include "Camera.h"
#include "Arduino.h"

camera_c::camera_c() {
		DDRK |= B00100000; // set output
		PORTK &= B11011111; // pull down
}

void camera_c::setPin(bool pinstate) {
	if(pinstate) {
		PORTK |= B00100000; // pull up
	} else {
		PORTK &= B11011111; // pull down
	}
}

void camera_c::shoot() {
	PORTK |= B00100000; // pull up
	delay(20);
	PORTK &= B11011111; // pull down
}
