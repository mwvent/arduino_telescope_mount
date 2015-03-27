// ------------------------------------------------------------------------
// Telescope Axis Class
// Handle The control, positioning and upkeep of a telescope axis
// equiv methods for ra and dec variants are paired up here
// ------------------------------------------------------------------------

#ifndef _axis_h_
#define _axis_h_

#include "settings.h"
#include "types.h"
#include "EEPROMHandler.h"
#include "encoders.h"
// library for the adafruit motor sheild
// http://www.ladyada.net/make/mshield/
#include <AFMotor.h>

#define _HISTORY_BYTES 24
#define _HISTORY_BYTES_HALF 12
#define _HISTORY_BYTES_QUATER 6
#define _ERROR_ZERO 0
#define _ERROR_WEST 1
#define _ERROR_NORTH 1
#define _ERROR_EAST 2
#define _ERROR_SOUTH 2
#define _ERROR_BIG_WEST 3
#define _ERROR_BIG_NORTH 3
#define _ERROR_BIG_EAST 4
#define _ERROR_BIG_SOUTH 4

class axis_c
{
	// Polling
	// -------
	public:
		virtual void update();
		virtual void priorityUpdate();
	
	// Object Pointers
	// ---------------
	protected:
		// the motor that controls the axis
		byte motorport;
		AF_DCMotor* motor;
		// a pointer to the function that provides encoder movement feedback
		int8_t* readEncoderMovement();
		// a pointer to the EEPROM Handler Object that handles settings for the motor
		EEPROMHandler_c* EEPROMHandler;
		// keep track of changes to the EEPROM settings area 
		int EEPROMUpdateIndex;
	
	// Gear Properties
	// ---------------
	public:
		// the amount of teeth on the main axis gear that the driven worm shaft drives 
		int gearTeeth; 
		// Encoder Pulses per revoultion of the worm gear
		unsigned int shaftPPR; 
		// Encoder Pulses per revolution of the axis
		signed long gearPPR; 
		// store (max=shaftPPR*gearTeeth) so dont need to recalc
		signed long gearPosition_max; 
		// store (max=shaftPPR*gearTeeth) / 2  so dont need to recalc
		signed long gearPosition_max_half; 
		
	// Position
	// --------
	public:
		// absolute position for the gear in  encoder pulses
		volatile signed long gearPosition_raw; 
		// the target gear position
		signed long gearPosition_target; 
		Angle* axisAngle;
		bool decWestSide;
		bool decEastSide;
		// an interrupt safe method for setting gearPosition_raw externally
		void setgearPosition_raw(signed long newValue);
		signed long newGearPositon_raw_value;
		bool newGearPositon_raw_value_event;
	protected:
		// get difference from A to B paying regards to the fact that the shortest route between the two may cross 0
		signed long getDiff(unsigned long A, unsigned long B);
		void resetgearPosErrHist();
		byte gearPosErrHist[_HISTORY_BYTES];
		byte gearPosErrHist_index;
	// Settings
	protected:
	        byte motorRange;
	        byte minimumMotorSpeed;
		byte errorTolerance;
};

// ------------------------------------------------------------------------
// RA Axis 
// Extend the axis class with right ascension specifics
// ------------------------------------------------------------------------

class ra_axis_c : public axis_c
{
	// Constructor
	// -----------
	public:
		ra_axis_c(byte imotorport, int encoder_PPR, int igearTeeth, EEPROMHandler_c* EEPROMObj);
	
	// Polling
	// -------
	public:
		void update();
		void priorityUpdate();
	
	// Position
	// --------
	public:
		// The gear position (with periodic error etc. applied)
		signed long gearPosition();
		// How far off target is the gear position
		signed long gearPositionError();
		// gearPosition converted to an angle
		Angle* currentAngle();
		// absolute position for the worm shaft driving the gear
		volatile signed int shaftPosition; 
		Angle* axisAngle;
		String Angle_text();
	private:
		// convert an Angle object to a raw gear position
		signed long AngleToGearPos(Angle posAngle);
		// real shaft position is not reflected in currentPos until first Z pulse
		volatile bool firstZrecorded; 
		
	// Control
	// -------
	public:
		void setSlewSpeed(byte speed);
		void slewEast(byte newSlewSpeed);
		void slewWest(byte newSlewSpeed);
		void stopSlew();
		bool isSlewing();
		// make current position the target position
		void sync();
		// make given position the current position
		void sync(Angle syncPos);
		// make given position the target position
		void setTarget(Angle syncPos);
		// set when the target is far away and unset when close
		bool goingTo;
	private:
		byte slewSpeed;
		bool slewingEast;
		bool slewingWest;
		// Set Motor Speed
		void setMotorSpeed(byte newSpeed);
		// Run Motor
		void runMotorEast();
		void runMotorEast(byte newSpeed) {
			setMotorSpeed(newSpeed);
			runMotorEast();
		}
		void runMotorWest();
		void runMotorWest(byte newSpeed) {
			setMotorSpeed(newSpeed);
			runMotorWest();
		}
		void stopMotor();
	
	// Tracking
	// --------
	public:
		// How long the shaft shold take to rotate at sidereel speed
		unsigned int sideReelShaftPeriod;
		// The shaft time is how many seconds from z index at sidereel speed the shaft is
		int shaftTime() {
			return (int)(((float)shaftPosition / (float)shaftPPR) * (float)sideReelShaftPeriod);
		}
	private:
		virtual void setupTrackingRates();
		// the delay(microseconds) between incrementing gearPosition_raw to counter rotation
		unsigned long sideRealPulseTime;
		// the delay(microseconds) between incrementing gearPosition_raw to counter rotation(with drift correction)
		unsigned long sideRealPulseTime_withDrift;
		// the time the last pulse was recorded
		unsigned long sideRealPulseTime_lastPulse;
};

// ------------------------------------------------------------------------
// DEC Axis 
// Extend the axis class with dec specifics
// ------------------------------------------------------------------------

class dec_axis_c : public axis_c
{
	// Constructor
	// -----------
	public:
		dec_axis_c(byte imotorport, int encoder_PPR, int igearTeeth, EEPROMHandler_c* EEPROMObj);
	
	// Polling
	// -------
	public:
		void update();
		void priorityUpdate();
				
	// Position
	// --------
	public:
		// gearPosition converted to an angle
		Angle* currentAngle();
		// The gear position (with periodic error etc. applied)
		signed long gearPosition();
		// How far off target is the gear position
		signed long gearPositionError();
		String Angle_text();
		bool DecWestSide();
	private:
		// convert an Angle object to a raw gear position
		signed long AngleToGearPos(Angle posAngle);
		
	// Control
	// -------
	public:
		void setSlewSpeed(byte speed);
		void slewNorth(byte newSlewSpeed);
		void slewSouth(byte newSlewSpeed);
		void stopSlew();
		bool isSlewing();
		// set when the target is far away and unset when close
		bool goingTo;
		// make current position the target position
		void sync();
		// make given position the current position
		void sync(Angle syncPos);
		// make given position the target position
		void setTarget(Angle syncPos);
	private:
		byte slewSpeed;
		bool slewingNorth;
		bool slewingSouth;
		// Set Motor Speed
		void setMotorSpeed(byte newSpeed);
		// Run Motor
		void runMotorNorth();
		void runMotorNorth(byte newSpeed) {
			setMotorSpeed(newSpeed);
			runMotorNorth();
		}
		void runMotorSouth();
		void runMotorSouth(byte newSpeed) {
			setMotorSpeed(newSpeed);
			runMotorSouth();
		}
		void stopMotor();
	
	// Tracking
	// -------
	private:
		// the delay(microseconds) between incrementing gearPosition_raw to counter drift
		unsigned long driftCorrectionPulseTime;
		// the time the last pulse was recorded (micros)
		unsigned long driftCorrectionPulseTime_lastPulse;	
		// the drift increments or decrements gear position
		bool driftCorrectionIsIncrement;
		virtual void setupTrackingRates();
		
};

#endif
