// ----------------------------------------------------------------------------------------------------------------
// Structures and types
// ----------------------------------------------------------------------------------------------------------------
#ifndef _types_h_
#define _types_h_
#include "settings.h"
#include "Arduino.h"
#include <math.h>

class Angle {
	public:
		int degrees;
		int arcHours;
		int arcMinutes;
		int arcSeconds;
		bool decWestSide;
		bool decEastSide;
		String RA_Angle_text();
		String DEC_Angle_text();
		float RAToDecimal();
		void SetRAFromGearPos(double GearPos, double GearPos_max);
		float DECToDecimal();
		void DecimalToDEC_fullCircle(float input);
};

class position {
	public:
		Angle RAPos;
		Angle DECPos;	
};

class location {
	public:
		Angle Lat;
		Angle Long;
};

class c_halfAngle // latitude and DEC values(-90 to +90)
{
	public:
		// value setting
		c_halfAngle(float value, bool isPercentage);
		c_halfAngle(int degreesVal, byte arcMinutesVal, byte arcSecondsVal);
		c_halfAngle(int degreesVal, float arcMinutesVal);
		// value conversion
		float toDecimal();
		float toPercentage();
		// values
		int degrees;
		byte arcMinutes;
		byte arcSeconds;
};

class c_fullAngle // longtide and RA values 360deg or 24hr
{
		// values
		bool isDegrees; // this flag specifies whever the below values are stored as deg'arcmin'arcsec or hh:mm:ss
		// ensuring this class can be used to store either type accuratley
		int degrees;
		byte arcMinutes;
		byte arcSeconds;
};
#endif
