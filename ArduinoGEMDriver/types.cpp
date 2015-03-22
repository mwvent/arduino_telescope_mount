#include "types.h"

String Angle::RA_Angle_text() {
	String TxtH = String(arcHours);
	if(arcHours<10) {
		TxtH = "0" + TxtH;
	}
	String TxtM = String(arcMinutes);
	if(arcMinutes<10) {
		TxtM = "0" + TxtM;
	}
	String TxtS = String(arcSeconds);
	if(arcSeconds<10) {
		TxtS = "0" + TxtS;
	}
	return TxtH + "h" + TxtM + "m" + TxtS + "s";
}

String Angle::DEC_Angle_text() {
	// convert degrees making sure always positive value (will add +/- after padding below)
	String TxtD;
	if(degrees<0) {
		TxtD = String(degrees - degrees - degrees);
	} else {
		TxtD = String(degrees);
	}
	if(degrees>-10 && degrees<10) {
		TxtD = "0" + TxtD;
	}
	if(degrees<0) {
		TxtD = "-" + TxtD;
	} else {
		TxtD = "+" + TxtD;
	}
	String TxtM = String(arcMinutes);
	if(arcMinutes<10) {
		TxtM = "0" + TxtM;
	}
	return TxtD + "D" + TxtM + "'";
	String TxtS = String(arcSeconds);
	if(arcSeconds<10) {
		TxtS = "0" + TxtS;
	}
	return TxtD + "D" + TxtM + "'" + TxtS + "s";
}

float Angle::RAToDecimal() { // 0-24 hour to 0-1
	float decimalRAcord = ((float)arcHours / 24.0);
	decimalRAcord += ((float)arcMinutes / 60.0) * (1.0 / 24.0 );
	decimalRAcord += ((float)arcSeconds / 60.0 / 60.0) * (1.0 / 24.0 );
	return decimalRAcord;
}

void Angle::SetRAFromGearPos(double GearPos, double GearPos_max) {
	static double GearPos_Hour_Value = (GearPos_max / (double)24);
	static double GearPos_Minute_Value = (GearPos_max / ((double)24 * (double)60) );
	static double GearPos_Second_Value = (GearPos_max / (((double)24 * (double)60) * (double)60) );
	double arcHours_double, arcMinutes_double, arcSeconds_double;
	arcHours_double = trunc(GearPos /  GearPos_Hour_Value);
	arcMinutes_double = trunc(GearPos / GearPos_Minute_Value);
	arcMinutes_double -= arcHours_double * (double)60;
	arcSeconds_double  = trunc(GearPos / GearPos_Second_Value);
	arcSeconds_double -= arcHours_double * (double)3600;
	arcSeconds_double -= arcMinutes_double * (double)60;
	arcHours = (int)arcHours_double;
	arcMinutes = (int)arcMinutes_double;
	arcSeconds = (int)arcSeconds_double;
	/*
	// if dec is West side add 12 hours
	if(decWestSide) {
		arcHours += 12;
	}
	// if over 24 hours wrap around
	if(arcHours >= 24) {
		arcHours -= 24;
	}
	*/
}

float Angle::DECToDecimal() { // return 0-1 representing % of 360deg circle
	float decimalDECcord =((float)degrees + 90) / 360.0;
	if(degrees < 0) {
		decimalDECcord += ((float)(60 - arcMinutes) / 60.0) * (1.0 / 360.0 );
		decimalDECcord += ((float)(60 - arcSeconds) / 60.0 / 60.0) * (1.0 / 360.0 );
	} else {
		decimalDECcord += ((float)arcMinutes / 60.0) * (1.0 / 360.0 );
		decimalDECcord += ((float)arcSeconds / 60.0 / 60.0) * (1.0 / 360.0 );
	}
	/*
	if(decWestSide) {
		decimalDECcord = 1.0 - decimalDECcord;
	}
	*/
	return decimalDECcord;
}

void Angle::DecimalToDEC_fullCircle(float input) {
	// half circle
	if(input > 0.5) {
		input = 1.0 - input;
		decEastSide = true;
		decWestSide = false;
	} else {
		decEastSide = false;
		decWestSide = true;
	}
	float breakdown = input * 360.0; // get degrees from 0 to 180
	degrees = (int)breakdown; // save degrees
	breakdown = (breakdown - (float)degrees) * 60.0; // remove degrees and multiply remainder by 60 to get arcminutes
	arcMinutes = (int)breakdown; // save arcMinutes
	breakdown = (breakdown - (float)arcMinutes) * 60.0; // remove arcMinutes and multiply remainder by 60 to get arcseconds
	arcSeconds = (int)breakdown; // save arcSeconds
	
	// now offset from 0
	degrees -= 90;
	if(degrees < 0) {
		arcMinutes = 60 - arcMinutes;
		arcSeconds = 60 - arcSeconds;
	}
}

c_halfAngle::c_halfAngle(float value, bool isPercentage) { // set angle from a percentage 0-1 or from a decimal -90.0 to +90.0
	if(!isPercentage) { // if not a percentage then convert to one before proceeding
		value = (value + 90.0) / 180.0;
	}
	// avoid divide by 0 and check for the easy answer
	if(value == 0.5) {
		degrees = 0;
		arcMinutes = 0;
		arcSeconds = 0;
		return;
	}
	bool isMinus = (value < 0.5);  // for the sake of simplicity work out -90 to 0 in inverse and set a flag to remember to invert it back
	if(isMinus) {
		value = 1.0 - value;    // now input is from range 0.5 to 1
	}
	value -= 0.5; // change to range 0.0 to 0.5 for simplicity
	float breakdown = value * 180.0; // get degrees from 0 to 90
	degrees = (int)breakdown; // save degrees
	breakdown = (breakdown - (float)degrees) * 60.0; // remove degrees and multiply remainder by 60 to get arcminutes
	arcMinutes = (byte)breakdown; // save arcMinutes
	breakdown = (breakdown - (float)arcMinutes) * 60.0; // remove arcMinutes and multiply remainder by 60 to get arcseconds
	arcSeconds = (byte)breakdown; // save arcSeconds
	if(isMinus) {
		degrees = 0 - degrees;    // make degrees negative if applicable
	}
}
c_halfAngle::c_halfAngle(int degreesVal, byte arcMinutesVal, byte arcSecondsVal) { // set parts individually
	degrees = degreesVal;
	arcMinutes = arcMinutesVal;
	arcSeconds = arcSecondsVal;
}
c_halfAngle::c_halfAngle(int degreesVal, float arcMinutesVal) { // as above but arcmin as a decimal
	degrees = degreesVal;
	arcMinutes = (byte)arcMinutesVal;
	float arcSecondsVal = (arcMinutesVal - (float)arcMinutes) * 60.0; // remainder * 60
	arcSeconds = (byte)arcSecondsVal;
}

// value conversion
float c_halfAngle::toDecimal() { // convert to decimal angle
	float retVal = (float)degrees;
	retVal += (float)arcMinutes / 60.0; // note: div by 0 returns 0 on arduino
	retVal += (float)arcSeconds / (60.0 * 60.0); // note: div by 0 returns 0 on arduino
	return retVal;
}

float c_halfAngle::toPercentage() { // convert to percentage 0=-90 0.5=0 1=90
	float retVal =((float)degrees + 90) / 180.0;
	retVal += ((float)arcMinutes / 60.0) * (1.0 / 180.0 );
	retVal += ((float)arcSeconds / 60.0 / 60.0) * (1.0 / 180.0 );
	return retVal;
}

/* notes
Longitudes traditionally have been written using "E" or "W"
instead of "+" or "−" to indicate this polarity. For example, the following all mean the same thing:
−91°, 91°W, +269°, 269°E.
*/

		// next stop http://www.stargazing.net/kepler/altaz.html
		/*
		static float LSTime() {
		  // LST = 100.46 + 0.985647 * d + long + 15*UT
		  // where d = days from J2000 (inc fraction of day), UT is universal time in decimal hours, long = longitude in decimal degrees
		};

		static  RADECtoALTAZ() {

		}; */