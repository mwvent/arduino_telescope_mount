// ------------------------------------------------------------------------
// Telescope Axis Class
// Handle The control, positioning and upkeep of a telescope axis
// equiv methods for ra and dec variants are paired up here
// ------------------------------------------------------------------------
#include "axis.h"
// -------------------------------------------------------------
// Init
// -------------------------------------------------------------

ra_axis_c::ra_axis_c(byte imotorport, int encoder_PPR, int igearTeeth, EEPROMHandler_c* EEPROMObj) {
	motorport = imotorport;
	motor = new AF_DCMotor(motorport, MOTOR12_1KHZ);
	EEPROMHandler = EEPROMObj;
	EEPROMUpdateIndex = EEPROMHandler->EEPROMUpdates;
	shaftPPR = (unsigned int)encoder_PPR;
	gearTeeth=igearTeeth;
	firstZrecorded=false;
	shaftPosition = -5000;
	gearPosition_max = (signed long)encoder_PPR * (signed long)gearTeeth;
	gearPosition_max_half = (signed long)((float)gearPosition_max / (float)2);
	gearPosition_target = gearPosition_max_half;
	gearPosition_raw = gearPosition_max_half;
	newGearPositon_raw_value_event = false;
	axisAngle = new Angle;
	setupTrackingRates();
	slewingEast = false;
	slewingWest = false;
	goingTo = false;
	resetgearPosErrHist();
}

dec_axis_c::dec_axis_c(byte imotorport, int encoder_PPR, int igearTeeth, EEPROMHandler_c* EEPROMObj) {
	motorport = imotorport;
	motor = new AF_DCMotor(motorport, MOTOR12_1KHZ);
	EEPROMHandler = EEPROMObj;
	EEPROMUpdateIndex = EEPROMHandler->EEPROMUpdates;
	shaftPPR = (unsigned int)encoder_PPR;
	gearTeeth=igearTeeth;
	gearPosition_max = (signed long)encoder_PPR * (signed long)gearTeeth;
	gearPosition_max_half = gearPosition_max / 2;
	gearPosition_target = gearPosition_max_half;
	gearPosition_raw = gearPosition_max_half;
	newGearPositon_raw_value_event = false;
	setupTrackingRates();
	slewingNorth = false;
	slewingSouth = false;
	slewSpeed = 0;
	axisAngle = new Angle;
	resetgearPosErrHist();
}

void axis_c::resetgearPosErrHist() {
	for(byte loop = 0; loop<(byte)_HISTORY_BYTES; loop++) {
		gearPosErrHist[loop] = _ERROR_ZERO;
	}
	gearPosErrHist_index = 0;
}

// Tracking Rates
void ra_axis_c::setupTrackingRates() {
	// seconds in a sidereal day
	float sideRealDayInSeconds = 23.9344699 * 60.0 * 60.0;
	// seconds shaft should take to rotate at sidereal speed
	float sideRealshaftPeriod_float = (sideRealDayInSeconds / (float)gearTeeth);
	// save shaft period
	sideReelShaftPeriod = (unsigned int)sideRealshaftPeriod_float;
	// get pulses per second moving at sidereal speed
	float pulseLengthInSeconds = sideRealshaftPeriod_float / (float)shaftPPR;
	// seconds to microseconds
	float pulseLengthInMicros = pulseLengthInSeconds * 1000000.0;
	// save rounded pulse time
	sideRealPulseTime = (unsigned long)pulseLengthInMicros;
	sideRealPulseTime_lastPulse=micros();
	errorTolerance = EEPROMHandler->readRAErrorTolerance();
	minimumMotorSpeed = EEPROMHandler->readRASlowSpeed();
	// minimumMotorSpeed = 0;
	motorRange = (byte)_RA_MOTOR_MAX_SPEED - (byte)minimumMotorSpeed;
}

void dec_axis_c::setupTrackingRates() {
	errorTolerance = EEPROMHandler->readDECErrorTolerance();
	minimumMotorSpeed = EEPROMHandler->readDECSlowSpeed();
	motorRange = (byte)_RA_MOTOR_MAX_SPEED - (byte)minimumMotorSpeed;
}

// -------------------------------------------------------------
// Main loop - High Priority - called at very regular intervals
// -------------------------------------------------------------
void ra_axis_c::priorityUpdate() {
	signed long errorTolerance_local = (signed long)(errorTolerance);
	signed long errorTolerance_local_neg = (signed long)(errorTolerance);
	
	// update gear position
	boolean zWasAlreadyKnown = firstZrecorded;
	gearPosition_raw += (signed long)encoders.readRAencoderMovement();
	if(newGearPositon_raw_value_event) {
	  gearPosition_raw = newGearPositon_raw_value;
	  newGearPositon_raw_value_event = false;
	}
	
	// check for flags where control of the motors via this routine is not desireable
	if(slewingEast || slewingWest) {
		return;
	}
	// get gear position error
	signed long gearPosErr = gearPositionError();
	// get error 'state'
	byte currentErrorState;
	if(gearPosErr == (signed long)0) { currentErrorState = (byte)_ERROR_ZERO; }
	if(gearPosErr < (signed long)0) { currentErrorState = (byte)_ERROR_WEST; }
	if(gearPosErr > (signed long)0) { currentErrorState = (byte)_ERROR_EAST; }
	if(gearPosErr < (signed long)-200) { currentErrorState = (byte)_ERROR_BIG_WEST; }
	if(gearPosErr > (signed long)200) { currentErrorState = (byte)_ERROR_BIG_EAST; }
	
	if(!goingTo) {
	        // update history
		gearPosErrHist_index++; 
		if(gearPosErrHist_index >= _HISTORY_BYTES) { gearPosErrHist_index = 0; }
		gearPosErrHist[gearPosErrHist_index] = currentErrorState;
	}
	
	switch(currentErrorState) {
	  case _ERROR_ZERO:
		motor->setSpeed(0);
		motor->run(RELEASE);
		goingTo = false;
		break;
	  case _ERROR_EAST:
		runMotorWest(1);
		goingTo = false;
		break;
	  case _ERROR_BIG_EAST:
		runMotorWest(255);
		goingTo = true;
		break;
	  case _ERROR_WEST:
		runMotorEast(1);
		goingTo = false;
		break;
	  case _ERROR_BIG_WEST:
		goingTo = true;
		runMotorEast(255);
		break;
	}
}

void dec_axis_c::priorityUpdate() {
	// update gear position
	gearPosition_raw += (signed long)encoders.readDECencoderMovement();
	if(newGearPositon_raw_value_event) {
	  gearPosition_raw = newGearPositon_raw_value;
	  newGearPositon_raw_value_event = false;
	}
	
	// no tracking when slewing
	if(slewingNorth || slewingSouth) {
		return;
	}
	
	// get error 'state'
	byte currentErrorState;
	signed long gearPosErr = gearPositionError();
	if(gearPosErr == (signed long)0) { currentErrorState = (byte)_ERROR_ZERO; }
	if(gearPosErr < (signed long)0) { currentErrorState = (byte)_ERROR_SOUTH; }
	if(gearPosErr > (signed long)0) { currentErrorState = (byte)_ERROR_NORTH; }
	if(gearPosErr < (signed long)-200) { currentErrorState = (byte)_ERROR_BIG_SOUTH; }
	if(gearPosErr > (signed long)200) { currentErrorState = (byte)_ERROR_BIG_NORTH; }
	
	if(!goingTo) {
	        // update history
		gearPosErrHist_index++; 
		if(gearPosErrHist_index >= _HISTORY_BYTES) { gearPosErrHist_index = 0; }
		gearPosErrHist[gearPosErrHist_index] = currentErrorState;
	}
	
	switch(currentErrorState) {
	  case _ERROR_ZERO:
		motor->setSpeed(0);
		motor->run(RELEASE);
		goingTo = false;
		break;
	  case _ERROR_NORTH:
		runMotorSouth(1);
		goingTo = false;
		break;
	  case _ERROR_BIG_NORTH:
		runMotorSouth(255);
		goingTo = true;
		break;
	  case _ERROR_SOUTH:
		runMotorNorth(1);
		goingTo = false;
		break;
	  case _ERROR_BIG_SOUTH:
		goingTo = true;
		runMotorNorth(255);
		break;
	}
}


// -------------------------------------------------------------
// Main loop - Low Priority
// -------------------------------------------------------------
void ra_axis_c::update() {
	priorityUpdate();
	// Update Shaft Position
	boolean zWasAlreadyKnown = firstZrecorded;
	switch(encoders.readRAshaftReset()) {
		case -1 :
			firstZrecorded = true;
			shaftPosition = 0 + (signed int)encoders.readRAshaftMovement();
			break;
		case 0 :
			shaftPosition += (signed int)encoders.readRAshaftMovement();
			break;
		case 1 :
			firstZrecorded = true;
			shaftPosition = shaftPPR + (signed int)encoders.readRAshaftMovement();
			break;
	}
	
	// if slewing just run the motors and return from function
	if(slewingEast || slewingWest) {
		setMotorSpeed(slewSpeed);
		if(slewingEast) {
			runMotorEast();
		}
		if(slewingWest) {
			runMotorWest();
		}
		sync();
		return;
	}
	
	// get a tally of error history
	byte history_count[3]= {0,0,0};
	for(byte loop=0; loop<(byte)_HISTORY_BYTES; loop++) {
		if(gearPosErrHist[loop] < _ERROR_BIG_WEST) {
			history_count[gearPosErrHist[loop]]++;
		}
	}
	// use error history tally to see if any motor adjustments may help
	if(history_count[_ERROR_ZERO] < _HISTORY_BYTES_QUATER) {
		if(history_count[_ERROR_EAST]>((byte)_HISTORY_BYTES_QUATER - 1 ) && 
		    history_count[_ERROR_WEST]>((byte)_HISTORY_BYTES_QUATER - 1 )) {
			// wiggling around target a lot - slow down motor
			minimumMotorSpeed--;
			motorRange = (byte)_RA_MOTOR_MAX_SPEED - (byte)minimumMotorSpeed;
			resetgearPosErrHist();
		}
		if(history_count[_ERROR_EAST]>((byte)_HISTORY_BYTES - 1 ) ||
		    history_count[_ERROR_WEST]>((byte)_HISTORY_BYTES - 1 )) {
			// loosing target - increase speed
			minimumMotorSpeed++;
			motorRange = (byte)_RA_MOTOR_MAX_SPEED - (byte)minimumMotorSpeed;
			resetgearPosErrHist();
		}
	}
	
	// if going to check target distance if close then unset
	if(goingTo) {
		if((gearPositionError() > -200) || gearPositionError() < 200) {
			goingTo = false;
		}
	}
	
	// Check for EEPROM Updates - if settings has changed then reload them
	if(EEPROMHandler->EEPROMUpdates != EEPROMUpdateIndex) {
		EEPROMUpdateIndex = EEPROMHandler->EEPROMUpdates;
		setupTrackingRates();
	}
	
	// sidereal motion
	// increment gearPosition_raw every sideReelPulseTime microseconds to counter sidereal motion
	// time of next event
	
	unsigned long nextPulse = (sideRealPulseTime_lastPulse + sideRealPulseTime);
	// check for overflow
	boolean microsOverflow = (micros() < 0x000000FF) && (nextPulse > 0xFFFFFF00);
	// timer
	if((micros() > nextPulse) | microsOverflow) {
		sideRealPulseTime_lastPulse += sideRealPulseTime;
		// sidereal motion has effectivley 'pushed' the gears effective position back West
		gearPosition_raw--;
		if(gearPosition_raw<0) {
			gearPosition_raw=(gearPosition_max-1);
		}
	}
}

void dec_axis_c::update() {
	priorityUpdate();
	
	// if slewing just run the motors, sync position and return from function
	if(slewingNorth || slewingSouth) {
		setMotorSpeed(slewSpeed);
		if(slewingNorth) {
			runMotorNorth();
		}
		if(slewingSouth) {
			runMotorSouth();
		}
		sync();
		return;
	}
	
	// Check for EEPROM Updates
	if(EEPROMHandler->EEPROMUpdates != EEPROMUpdateIndex) {
		EEPROMUpdateIndex = EEPROMHandler->EEPROMUpdates;
		setupTrackingRates();
	}
	
	// get a tally of error history
	byte history_count[3]= {0,0,0};
	for(byte loop=0; loop<(byte)_HISTORY_BYTES; loop++) {
		if(gearPosErrHist[loop] < _ERROR_BIG_WEST) {
			history_count[gearPosErrHist[loop]]++;
		}
	}
	// use error history tally to see if any motor adjustments may help
	if(history_count[_ERROR_ZERO] < _HISTORY_BYTES_QUATER) {
		if(history_count[_ERROR_NORTH]>((byte)_HISTORY_BYTES_QUATER - 1 ) && 
		    history_count[_ERROR_SOUTH]>((byte)_HISTORY_BYTES_QUATER - 1 )) {
			// wiggling around target a lot - slow down motor
			minimumMotorSpeed--;
			motorRange = (byte)_DEC_MOTOR_MAX_SPEED - (byte)minimumMotorSpeed;
			resetgearPosErrHist();
		}
		if(history_count[_ERROR_NORTH]>((byte)_HISTORY_BYTES - 1 ) ||
		    history_count[_ERROR_SOUTH]>((byte)_HISTORY_BYTES - 1 )) {
			// loosing target - increase speed
			minimumMotorSpeed++;
			motorRange = (byte)_DEC_MOTOR_MAX_SPEED - (byte)minimumMotorSpeed;
			resetgearPosErrHist();
		}

	}
}

// -------------------------------------------------------------
// Position
// -------------------------------------------------------------
signed long ra_axis_c::gearPosition() {
		return gearPosition_raw;
}

signed long dec_axis_c::gearPosition() {
		return gearPosition_raw;
}

void axis_c::setgearPosition_raw(signed long newValue) {
	newGearPositon_raw_value = newValue;
	newGearPositon_raw_value_event = true;
}

signed long ra_axis_c::gearPositionError() {
	return getDiff(gearPosition(), gearPosition_target);
}

signed long dec_axis_c::gearPositionError() {
	return getDiff(gearPosition(), gearPosition_target);
}


Angle* ra_axis_c::currentAngle() {
	axisAngle -> SetRAFromGearPos((double)gearPosition(), (double)gearPosition_max);
	return axisAngle;
}

Angle* dec_axis_c::currentAngle() {
	axisAngle -> DecimalToDEC_fullCircle((float)gearPosition() / (float)gearPosition_max);
	return axisAngle;
}

bool dec_axis_c::DecWestSide() {
  if(gearPosition_raw > gearPosition_max_half) {
	axisAngle -> decWestSide = false;
	axisAngle -> decEastSide = true;
	return false;
  } else {
	axisAngle -> decWestSide = true;
	axisAngle -> decEastSide = false;
	return true;
  }
}

// get difference from A to B paying regards to the fact that the shortest route between the two may cross 0
signed long axis_c::getDiff(unsigned long A, unsigned long B) {
  // cache pervious result to avoid wasting cycles recalcing
	static signed long lastResult;
	static signed long lastA;
	static signed long lastB;

	// check if result has already been calculated and return if it has
	if((lastA==A)&&(lastB==B)) {
		return lastResult;
	}

	// otherwise re-calc
	lastA = A;
	lastB = B;
	signed long rawDiff = B - A; // the RAW difference

	// if less than half the circle away from each other the 0 point has not been crossed so rawDiff is result
	if(!((rawDiff > gearPosition_max_half) || ((0-rawDiff) > gearPosition_max_half))) {
		lastResult=rawDiff;
		return lastResult;
	}
	if(A > B) {
		lastResult = (gearPosition_max - A) + B;    // B is in front of A over the 0 line
		return lastResult;
	}
	// B must be greater then A
	lastResult = 0-((gearPosition_max - B) + A); // B is behind A over the 0 line
	return lastResult;
}

signed long ra_axis_c::AngleToGearPos(Angle posAngle) {
	return (signed long)(posAngle.RAToDecimal() * (float)gearPosition_max);
}

signed long dec_axis_c::AngleToGearPos(Angle posAngle) {
	return (signed long)(posAngle.DECToDecimal() * (float)gearPosition_max);
}

// -------------------------------------------------------------
// Control
// -------------------------------------------------------------
// make current position the target
void ra_axis_c::sync() {
	gearPosition_target = gearPosition();
}

void dec_axis_c::sync() {
	gearPosition_target = gearPosition();
}

// make specified Angle object the current position
void ra_axis_c::sync(Angle syncPos) {
	gearPosition_raw = AngleToGearPos(syncPos);
	sync();
}

void dec_axis_c::sync(Angle syncPos) {
	gearPosition_raw = AngleToGearPos(syncPos);
	sync();
}

// make specified Angle object the target
void ra_axis_c::setTarget(Angle syncPos) {
	gearPosition_target = AngleToGearPos(syncPos);
}

void dec_axis_c::setTarget(Angle syncPos) {
	gearPosition_target = AngleToGearPos(syncPos);
}

// ra slewing/manual control
void ra_axis_c::stopSlew() {
	if(slewingEast || slewingWest) {
		slewSpeed = 0;
		// unset slewing flags
		slewingEast = false;
		slewingWest = false;
		stopMotor();
	}
}
// dec slewing/manual control
void dec_axis_c::stopSlew() {
	if(slewingNorth || slewingSouth) {
		slewSpeed = 0;
		// unset slewing flags
		slewingNorth = false;
		slewingSouth = false;
		stopMotor();
	}
}

bool ra_axis_c::isSlewing() {
	return (slewingWest || slewingEast);
}

bool dec_axis_c::isSlewing() {
	return (slewingNorth || slewingSouth);
}

void ra_axis_c::slewEast(byte newSlewSpeed) {
	slewSpeed = newSlewSpeed;
	slewingEast = true;
	slewingWest = false;
}

void ra_axis_c::slewWest(byte newSlewSpeed) {
	slewSpeed = newSlewSpeed;
	slewingEast = false;
	slewingWest = true;
}

void dec_axis_c::slewNorth(byte newSlewSpeed) {
	slewSpeed = 0;
	slewingNorth = true;
	slewingSouth = false;
}

void dec_axis_c::slewSouth(byte newSlewSpeed) {
	slewSpeed = 0;
	slewingNorth = false;
	slewingSouth = true;
}

// motor speed setting
void ra_axis_c::setMotorSpeed(byte newSpeed) {
	/*
	 * Old code is a tad resource intenstive, replacing with something simpler
	 * where motor speed is offset by minimumMotorSpeed and once hits ceiling
	 * then its tough - lets see how that goes
	 */
	/*
	float calcSpeed = (float)minimumMotorSpeed + ( (float)motorRange * ( (float)newSpeed / 255.0 ) );
	byte finalSpeed = (byte)calcSpeed;
	// logically useless but double check for safetey - dont want more burnt out HBridges
	if(finalSpeed > (byte)_RA_MOTOR_MAX_SPEED) {
		finalSpeed = (byte)_RA_MOTOR_MAX_SPEED;
	}
	motor->setSpeed(finalSpeed);
	*/
	// check newSpeed+minimumMotorSpeed overflow 8bit limit
	if(((int)newSpeed + (int)minimumMotorSpeed) > 255) {
		motor->setSpeed((byte)_RA_MOTOR_MAX_SPEED);
		return;
	}
	// now we know it doesnt overflow offset newSpeed with minimumMotorSpeed
	newSpeed += minimumMotorSpeed;
	// check hit ceiling
	if(newSpeed > (byte)_RA_MOTOR_MAX_SPEED) {
		motor->setSpeed((byte)_RA_MOTOR_MAX_SPEED);
		return;
	}
	
	// otherwise set newSpeed + minimumMotorSpeed
	motor->setSpeed(newSpeed);
}

void dec_axis_c::setMotorSpeed(byte newSpeed) {
	// check newSpeed+minimumMotorSpeed overflow 8bit limit
	if(((int)newSpeed + (int)minimumMotorSpeed) > 255) {
		motor->setSpeed((byte)_DEC_MOTOR_MAX_SPEED);
		return;
	}
	// now we know it doesnt overflow offset newSpeed with minimumMotorSpeed
	newSpeed += minimumMotorSpeed;
	// check hit ceiling
	if(newSpeed > (byte)_DEC_MOTOR_MAX_SPEED) {
		motor->setSpeed((byte)_DEC_MOTOR_MAX_SPEED);
		return;
	}
	// otherwise set newSpeed + minimumMotorSpeed
	motor->setSpeed(newSpeed);
}

// Run Motor
void ra_axis_c::stopMotor() {
	motor->run(RELEASE);
}

void dec_axis_c::stopMotor() {
	motor->run(RELEASE);
}

void dec_axis_c::runMotorNorth() {
	#ifndef _DEC_DIRECTION_REVERSE
		motor->run(BACKWARD);
	#else
		motor->run(FORWARD);
	#endif
}

void dec_axis_c::runMotorSouth() {
	#ifndef _DEC_DIRECTION_REVERSE
		motor->run(FORWARD);
	#else
		motor->run(BACKWARD);
	#endif
}

void ra_axis_c::runMotorEast() {
	#ifndef _RA_DIRECTION_REVERSE
		motor->run(BACKWARD);
	#else
		motor->run(FORWARD);
	#endif
}

void ra_axis_c::runMotorWest() {
	#ifndef _RA_DIRECTION_REVERSE
		motor->run(FORWARD);
	#else
		motor->run(BACKWARD);
	#endif
}




