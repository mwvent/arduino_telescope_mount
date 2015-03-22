// ------------------------------------------------------------------------
// Telescope Mount Class
// A telescope mount is an RA and DEC axis class
// This class provides methods that need to work on both axis
// ------------------------------------------------------------------------

#include "mount.h"

mount_c::mount_c(uint8_t RAmotorPort, uint8_t DECmotorPort,
				int RAGearTeeth, int DECGearTeeth,
				int RAEncoderPPR, int DECEncoderPPR,  
				DS1307_RTC_c* DS1307_RTC_Obj,	EEPROMHandler_c* EEPROMObj) {
	
	DS1307_RTC = DS1307_RTC_Obj;
	EEPROMHandler = EEPROMObj;				
	ra_axis = new ra_axis_c(RAmotorPort, RAEncoderPPR, RAGearTeeth, EEPROMObj);
	dec_axis = new dec_axis_c(DECmotorPort, DECEncoderPPR, DECGearTeeth, EEPROMObj);
	encoders.begin(ra_axis, dec_axis);
}

// -------------------------------------------------------------
// Control
// -------------------------------------------------------------
void mount_c::update() {
	// keep ra_axis informed if dec has crossed meridian
        // this will only work when another class is reading the co-ordinates using the
        // currentAngle class (ie LX200Serial & UI) 
	//ra_axis -> axisAngle -> decEastSide = !dec_axis -> DecWestSide();
	//ra_axis -> axisAngle -> decWestSide = dec_axis -> DecWestSide();
	// update
	ra_axis->update();
	dec_axis->update();
}

// slew commmands
void mount_c::stopSlew() {
	ra_axis->stopSlew();
	dec_axis->stopSlew();
}

void mount_c::slewNorth(byte speed) {
	dec_axis->slewNorth(speed);
}

void mount_c::slewSouth(byte speed) {
	dec_axis->slewSouth(speed);
}

void mount_c::slewEast(byte speed) {
	ra_axis->slewEast(speed);
}

void mount_c::slewWest(byte speed) {
	ra_axis->slewWest(speed);
}

// goto and position set
void mount_c::sync() {
	ra_axis -> sync();
	dec_axis -> sync();
}
void mount_c::sync(position syncPos) {
	ra_axis -> sync(syncPos.RAPos);
	dec_axis -> sync(syncPos.DECPos);
}

void mount_c::gotoPos(position gotoPos) {
	// check if crossing the meridian is needed?
	// check if target DEC pos is west side or east side
        /*
	if((double)gotoPos.RAPos.arcHours >= (double)12.0) { // this wont work, depends on current pos
		gotoPos.DECPos.decWestSide = true;
		gotoPos.DECPos.decEastSide = false;
	} else {
		gotoPos.DECPos.decWestSide = false;
		gotoPos.DECPos.decEastSide = true;
	}
	// check if mount is currentley inverse to the east or west of the target
	if(gotoPos.DECPos.decWestSide != dec_axis -> currentAngle() -> decWestSide) {
		// if it is flip the target RA by 12 hours -  this will not change the 'actual' target
		// but compensates for the 'flip' that will happen when dec slews over the line
		gotoPos.RAPos.arcHours += 12;
		if(gotoPos.RAPos.arcHours > 24) {
			gotoPos.RAPos.arcHours -= 24;
		}
	}*/
	ra_axis -> setTarget(gotoPos.RAPos);
	dec_axis -> setTarget(gotoPos.DECPos);
}

// move target for finer control
// move target position
void mount_c::moveTarget(int RAOffset, int DECOffset)
{
	ra_axis->gearPosition_target += RAOffset;
	dec_axis->gearPosition_target += DECOffset;
}


// for direct joystick control, x=ra from -255 to 255 y=dec from -255 to 255
// only the axis with the highest value is moved
void mount_c::joystickControl(int x, int y, bool fast) {
	
	int muliplier=1;
	// get positive values for x and y if they are negative
	int xPositive;
	int yPositive;
	if(x<0) {
		xPositive = 0 - x;
	} else {
		xPositive = x;
	}
	if(y<0) {
		yPositive = 0 - y;
	} else {
		yPositive = y;
	}
	// Altough the joystick could be used to control two axis at once
	// we only want to move one axis at a time with the joystick, it
	// is rather hard to control otherwise
	// so flag to check if x axis is dominant or larger value
	bool xDominant=xPositive > yPositive;
	// set multiplier
	if(fast) {
		muliplier=6;
	}
	
	// if both axis 0 (joystick center)
	static bool lastTimeWasZeros = false;
	if(x==0 && y==0) {
		// send stops but only if havent been sent previous 
	        // (prevent halting slews from LX200Serial or other)
		if(!lastTimeWasZeros) {
			ra_axis -> stopSlew();
			dec_axis -> stopSlew();
			lastTimeWasZeros = true;
		}
	} else {
		lastTimeWasZeros = false;
	}
	// Altough the joystick could be used to control two axis at once
	// we only want to move one axis at a time with the joystick, it
	// is rather hard to control otherwise - check if x is 'dominant'
	// and slew in that direction only
	// x value is dominant
	if(x!=0 && xDominant) {
		// calc speed
		byte slewSpeed;
		if(fast) {
			slewSpeed = (byte)((float)xPositive * 2.55);
		} else {
			slewSpeed = (byte)xPositive ;
		}
		// pass to axis
		if(x > 0) {
			ra_axis->slewWest(slewSpeed);
		} else {
			ra_axis->slewEast(slewSpeed);
		}
	} else {
		// stop axis slew if not dominant
		ra_axis->stopSlew();
	}
	// y value is dominant
	if(y!=0 && (!xDominant)) {
		// calc speed
		byte slewSpeed;
		if(fast) {
			slewSpeed = (byte)((float)yPositive * 2.55);
		} else {
			slewSpeed = (byte)yPositive;
		}
		// pass to axis
		if(y > 0) {
			dec_axis->slewNorth(slewSpeed);
		} else {
			dec_axis->slewSouth(slewSpeed);
		}
	} else {
		// stop axis slew if not dominant
		dec_axis->stopSlew();
	}
}

