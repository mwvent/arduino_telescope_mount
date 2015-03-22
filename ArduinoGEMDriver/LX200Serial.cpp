#include "LX200Serial.h"

LX200SerialHandler_c::LX200SerialHandler_c(mount_c* mountobj,  DS1307_RTC_c* DS1307_RTCObj, EEPROMHandler_c* EEPROMObj)
{
	serialBuffer_count=0;
	motionRate = 10;
	mount = mountobj;
	DS1307_RTC = DS1307_RTCObj;
	EEPROMHandler = EEPROMObj;
	highPrecision=true;
	twentyFour_hourmode=true;
	longFormat=true;
	#ifdef __AVR_ATmega2560__
		Serial1.begin(9600);
	#endif
	Serial.begin(9600);
}

// -------------------------------------------------------------
// Serial Handling Wrappers / Helpers
// -------------------------------------------------------------
void LX200SerialHandler_c::SerialSend(String sendText)
{
	Serial1.print(sendText);
	Serial.print(sendText);
}

void LX200SerialHandler_c::SerialWrite(byte sendByte)
{
	Serial.write(sendByte);
	Serial1.write(sendByte);
}

void LX200SerialHandler_c::printIntLeadingZero(int value) {
	if(value<10) {
		SerialSend("0");
	}
	SerialSend(String(value));
}

void LX200SerialHandler_c::printIntLeadingZeroAndPolarity(int value) {
	if(value<0) {
		progmemPrint(PSTR("-"));    // print polarity
	} else {
		progmemPrint(PSTR(""));
	}
	if(value<0) {
		value = 0 - value;    // make positive
	}
	printIntLeadingZero(value);
}

int LX200SerialHandler_c::findCharacterAndChangeToStringTerminator(char* chars, int startPos, char charToFind) {
	for(int findCharPos=startPos; true; findCharPos++) {
		if((chars[findCharPos]=='#' && charToFind!='#') || chars[findCharPos] == 0) { // found # or string terminator
			return -1; // return invalid
		}
		if(chars[findCharPos] == charToFind) { // found ASCII 223 (degree symbol)
			chars[findCharPos]=0; // terminate string
			return findCharPos;
		}
	}
}

void LX200SerialHandler_c::progmemPrint (PGM_P s) {
	char c;
	while ((c = pgm_read_byte(s++)) != 0) {
		Serial.print(c);
		Serial1.print(c);
	}
}

void LX200SerialHandler_c::progmemPrintln (PGM_P s) {
	char c;
	while ((c = pgm_read_byte(s++)) != 0) {
		Serial.print(c);
		Serial1.print(c);
	}
	Serial1.println("");
	Serial.println("");
}

// -------------------------------------------------------------
// Print LX200 Format Co-ordinates
// -------------------------------------------------------------
void LX200SerialHandler_c::printLX200positionRA(Angle* pos) {
	// HH:
	printIntLeadingZero(pos -> arcHours);
	SerialSend(":");
	// MM
	printIntLeadingZero(pos -> arcMinutes);
	// :SS if long format enabled
	if(longFormat) {
		SerialSend(":");
		printIntLeadingZero(pos -> arcSeconds);
	}
	// .xx (tenths of minute) if short format
	if(!longFormat) {
		SerialSend(".");
		byte tenthsOfMinute=0;
		tenthsOfMinute = (byte)(((float)pos -> arcSeconds / 60.0) * 10.0);
		SerialSend(String(tenthsOfMinute));
	}
	SerialSend("#");
}

void LX200SerialHandler_c::printLX200positionDEC(Angle* pos) {
	// sdd  - the s is polarity (+ or -)
	printIntLeadingZeroAndPolarity(pos -> degrees);
	// ASCII 223 is the LX200's degree symbol and must be sent as a byte not a char
	SerialWrite(223);
	// MM
	printIntLeadingZero(pos -> arcMinutes);
	if(longFormat) { // 'SS if long format
		progmemPrint(PSTR("'"));
		printIntLeadingZero(pos -> arcSeconds);
	}
	progmemPrint(PSTR("#"));
}

// -------------------------------------------------------------
// Interpret LX200 Time Set :SL13:00:00#
// -------------------------------------------------------------
bool LX200SerialHandler_c::setLocalTime() {
	int hh;
	int mm;
	int ss;
	DateTime curTime = DS1307_RTC->getTime();
	int startPos=4;
	int findCharPos;
	if(serialBuffer[3]!=' ') {
		startPos=3;    // some clients dont send a space between :command and first digit
	}
	findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, ':');
	// no : symbol = invalid
	if(findCharPos==-1) {
		return false;
	}
	// save hh
	hh = atoi(&serialBuffer[startPos]);
	// find next :
	startPos = findCharPos + 1;
	findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, ':');
	if(findCharPos!=-1) {
		mm = atoi(&serialBuffer[startPos]);
		startPos = findCharPos + 1;
		// findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, '#');
		ss = atoi(&serialBuffer[startPos]);
		// set time
		DateTime setTime(curTime.year(), curTime.month(), curTime.day(), hh, mm, ss);
		DS1307_RTC->setTime(setTime);
		return true; // return ok
	}
	// could not determine format as no second :
	if(findCharPos == -1) {
		return false; // return fail
	}
}

bool LX200SerialHandler_c::setDate() {
	int yy;
	int mm;
	int dd;
	DateTime curTime = DS1307_RTC->getTime();
	int startPos=4;
	int findCharPos;
	if(serialBuffer[3]!=' ') {
		startPos=3;    // some clients dont send a space between :command and first digit
	}
	findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, '/');
	// no : symbol = invalid
	if(findCharPos==-1) {
		return false;
	}
	// save mm
	mm = atoi(&serialBuffer[startPos]);
	// find next /
	startPos = findCharPos + 1;
	findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, '/');
	if(findCharPos!=-1) {
		dd = atoi(&serialBuffer[startPos]);
		startPos = findCharPos + 1;
		findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, '#');
		yy = atoi(&serialBuffer[startPos]);
		// set time
		DateTime setTime(yy, mm, dd,  curTime.hour(), curTime.minute(), curTime.second());
		DS1307_RTC->setTime(setTime);
		return true; // return ok
	}
	// could not determine format as no second /
	if(findCharPos == -1) {
		return false; // return fail
	}
}

// -------------------------------------------------------------
// Interpret LX200 Format Co-ordinates :Sr03:04:05#
// -------------------------------------------------------------
bool LX200SerialHandler_c::setRA() {
	int startPos=4;
	int findCharPos;
	if(serialBuffer[3]!=' ') {
		startPos=3;    // some clients dont send a space between :Sr and digit
	}
	// find :
	findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, ':');
	// no : symbol = invalid
	if(findCharPos==-1) {
		return false;
	}
	// save hh
	currentObjectPos.RAPos.arcHours = atoi(&serialBuffer[startPos]);
	// look for short format - find decimal point .
	startPos = findCharPos + 1;
	findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, '.');
	if(findCharPos!=-1) {
		// found decimal point - fairly safe to assume short format HH:MM.s - where .s is tenths of a min
		currentObjectPos.RAPos.arcMinutes = atoi(&serialBuffer[startPos]);
		startPos = findCharPos + 1;
		findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, '#');
		// TODO below line assumes one digit - this should be OK though as >1 digit is over spec
		currentObjectPos.RAPos.arcSeconds = (int)60.0 * ((float)atoi(&serialBuffer[startPos]) * 0.1); // convert&save
		return true; // return ok
	}
	// look for long format - find :
	findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, ':');
	if(findCharPos!=-1) {
		currentObjectPos.RAPos.arcMinutes = atoi(&serialBuffer[startPos]);
		startPos = findCharPos + 1;
		findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, '#');
		currentObjectPos.RAPos.arcSeconds = atoi(&serialBuffer[startPos]);
		return true; // return ok
	}
	// could not determine format as no decimal point or :
	if(findCharPos == -1) {
		return false;
	}
}

bool LX200SerialHandler_c::setDEC() {
	int findCharPos;
	int startPos = 0;
	int degreePos;
	// +/-DD
	if(serialBuffer[4]=='+' || serialBuffer[4]=='-') {
		startPos = 4;    // check has +/- at position 4
	}
	if(serialBuffer[3]=='+' || serialBuffer[3]=='-') {
		startPos = 3;    // check has +/- at position 3
	}
	if(startPos == 0) { // +/- not found at pos 3 or 4 error
		return false; // return fail
	}
	// try and find degree (ASCII 223) symbol relative to starting pos
	findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, (unsigned byte)223);
	// if not found try alterative * symbol used by some clients (SkySafari)
	if(findCharPos==-1) { // invalid degree - try asterix (*) as used by sky safari
		findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, '*');
	}
	// Still no degree symbol found return error
	if(findCharPos==-1) {
		return false; // return fail
	}
	// read degrees
	currentObjectPos.DECPos.degrees = atoi(&serialBuffer[startPos]);
	// store position
	startPos = findCharPos + 1;
	degreePos = findCharPos;

	// try and find : symbol long format (sDD*MM:SS#) // TODO check for valid MM + SS?
	findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, ':');
	if(findCharPos!=-1) {
		currentObjectPos.DECPos.arcMinutes = atoi(&serialBuffer[startPos]);
		startPos = findCharPos + 1;
		findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], startPos, '#');
		currentObjectPos.DECPos.arcSeconds = atoi(&serialBuffer[startPos]);
		return true; // return ok
	}

	// : symbol NOT found - assume short format (sDD*MM#) // TODO check for valid MM?
	findCharPos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], degreePos + 1, '#');
	currentObjectPos.DECPos.arcMinutes = atoi(&serialBuffer[degreePos + 1]);
	currentObjectPos.DECPos.arcSeconds = 0;
	return true; // return ok
}

// -------------------------------------------------------------
// Main Loop
// -------------------------------------------------------------
void LX200SerialHandler_c::update() {
	while (Serial1.available() > 0) {
		serialBuffer[serialBuffer_count] =  Serial1.read();
		
		// ignore CR LF
		if(serialBuffer[serialBuffer_count]==10) {
			return;
		}
		if(serialBuffer[serialBuffer_count]==13) {
			return;    
		}
		// ignore invalid starting characters
		if(serialBuffer_count==0) {
			if(serialBuffer[0]!=':') {
				return;
			}
		}

		if(serialBuffer[serialBuffer_count] == '#') { // hit a hash
			serialBuffer[serialBuffer_count] = 0;
			#ifdef debugOut_showSPI1commandsOnSPI0
			// send a copy of the command rcvd to SPI0
			Serial.print("Received: '");
			for(byte lp=0; lp<=serialBuffer_count; lp++) {
				Serial.write(serialBuffer[lp]);
			}
			Serial.println("'");
			#endif
			// process command unless the hash was the only thing received
			if(serialBuffer_count!=0) { 
				#ifdef debugOut_showSPI1commandsOnSPI0
				Serial.print("Sent: '");
				#endif
				serialCommand();
				#ifdef debugOut_showSPI1commandsOnSPI0
				Serial.println("'");
				#endif
			}
			serialBuffer_count=0;
		} else {
			if(serialBuffer_count<25) {
				serialBuffer_count++;
			} else {
				// progmemPrint(PSTR("Serial Buffer overflow#"));
			}
		}
	}
	while (Serial.available() > 0) {
		serialBuffer[serialBuffer_count] =  Serial.read();
		if(serialBuffer[serialBuffer_count]==10) {
			return;
		}
		if(serialBuffer[serialBuffer_count]==13) {
			return;    // ignore CR LF
		}

		if(serialBuffer[serialBuffer_count] == '#') { // hit a hash
			serialBuffer[serialBuffer_count] = 0;
			if(serialBuffer_count!=0) { // process command unless the hash was the only thing received
				serialCommand();
			}
			serialBuffer_count=0;
		} else {
			if(serialBuffer_count<25) {
				serialBuffer_count++;
			} else {
				progmemPrint(PSTR("Serial Buffer overflow#"));
			}
		}
	}
}

// -------------------------------------------------------------
// Command parser
// -------------------------------------------------------------
enum LX200Commands {
	other, ACK, P_HighPrecisionToggle,
	U_ToggleLongFormat, 
	// Motion Rates
	RG_MotionRateGuide, RC_MotionRateCenter, RM_MotionRateFind, RS_MotionRateSlew,
	// Motion
	MS_SlewToCurrentObject, Q_AbortSlew,
	Mn_MoveNorth, Ms_MoveSouth, Me_MoveEast, Mw_MoveWest,
	Qn_StopMoveNorth, Qs_StopMoveSouth, Qe_StopMoveEast, Qw_StopMoveWest,
	// Position
	GR_GetRAOfMount, GD_GetDECOfMount,
	Sr_SetRAOfCurrentObject, Sd_SetDECOfCurrentObject,
	CM_SyncToCurrentObject,
	// Date & Time
	c_Get12or24HourClockStatus, GS_GetSiderealTime, SS_SetSiderealTime,
	GL_GetLocalTime, Ga_GetLocalTime, SL_SetLocalTime, GC_GetCalendarDate,
	SC_SetCalendarDate,
	// Location
	SG_SetOffsetFromGMT,
	St_SetLatitude, Sg_SetLongitude,
	Gt_GetLatitude, Gg_GetLongitude,
};
void LX200SerialHandler_c::serialCommand() {
	DateTime nowTime = DS1307_RTC->getTime();
	int loopInt = 0;
	LX200Commands currentCommand = other;
	if(serialBuffer[1]=='A'&&serialBuffer[2]=='C'&&serialBuffer[3]=='K') {
		currentCommand=ACK;
	}
	if(serialBuffer[1]=='P') {
		currentCommand=P_HighPrecisionToggle;
	}
	if(serialBuffer[1]=='U') {
		currentCommand=U_ToggleLongFormat;
	}
	if(serialBuffer[1]=='C' && serialBuffer[2]=='M') {
		currentCommand=CM_SyncToCurrentObject;
	}
	if(serialBuffer[1]=='M' && serialBuffer[2]=='S') {
		currentCommand=MS_SlewToCurrentObject;
	}
	if(serialBuffer[1]=='Q') {
		currentCommand=Q_AbortSlew;
	}
	if(serialBuffer[1]=='S' && serialBuffer[2]=='r') {
		currentCommand=Sr_SetRAOfCurrentObject;
	}
	if(serialBuffer[1]=='S' && serialBuffer[2]=='d') {
		currentCommand=Sd_SetDECOfCurrentObject;
	}
	if(serialBuffer[1]=='G' && serialBuffer[2]=='R') {
		currentCommand=GR_GetRAOfMount;
	}
	if(serialBuffer[1]=='G' && serialBuffer[2]=='D') {
		currentCommand=GD_GetDECOfMount;
	}
	if(serialBuffer[1]=='G' && serialBuffer[2]=='S') {
		currentCommand=GS_GetSiderealTime;
	}
	if(serialBuffer[1]=='S' && serialBuffer[2]=='S') {
		currentCommand=SS_SetSiderealTime;
	}
	if(serialBuffer[1]=='G' && serialBuffer[2]=='L') {
		currentCommand=GL_GetLocalTime;
	}
	if(serialBuffer[1]=='G' && serialBuffer[2]=='a') {
		currentCommand=Ga_GetLocalTime;
	}
	if(serialBuffer[1]=='S' && serialBuffer[2]=='L') {
		currentCommand=SL_SetLocalTime;
	}
	if(serialBuffer[1]=='G' && serialBuffer[2]=='C') {
		currentCommand=GC_GetCalendarDate;
	}
	if(serialBuffer[1]=='S' && serialBuffer[2]=='C') {
		currentCommand=SC_SetCalendarDate;
	}
	if(serialBuffer[1]=='M' && serialBuffer[2]=='n') {
		currentCommand=Mn_MoveNorth;
	}
	if(serialBuffer[1]=='M' && serialBuffer[2]=='s') {
		currentCommand=Ms_MoveSouth;
	}
	if(serialBuffer[1]=='M' && serialBuffer[2]=='e') {
		currentCommand=Me_MoveEast;
	}
	if(serialBuffer[1]=='M' && serialBuffer[2]=='w') {
		currentCommand=Mw_MoveWest;
	}
	if(serialBuffer[1]=='Q' && serialBuffer[2]=='n') {
		currentCommand=Qn_StopMoveNorth;
	}
	if(serialBuffer[1]=='Q' && serialBuffer[2]=='s') {
		currentCommand=Qs_StopMoveSouth;
	}
	if(serialBuffer[1]=='Q' && serialBuffer[2]=='e') {
		currentCommand=Qe_StopMoveEast;
	}
	if(serialBuffer[1]=='Q' && serialBuffer[2]=='w') {
		currentCommand=Qw_StopMoveWest;
	}
	if(serialBuffer[1]=='R' && serialBuffer[2]=='G') {
		currentCommand=RG_MotionRateGuide;
	}
	if(serialBuffer[1]=='R' && serialBuffer[2]=='C') {
		currentCommand=RC_MotionRateCenter;
	}
	if(serialBuffer[1]=='R' && serialBuffer[2]=='M') {
		currentCommand=RM_MotionRateFind;
	}
	if(serialBuffer[1]=='R' && serialBuffer[2]=='S') {
		currentCommand=RS_MotionRateSlew;
	}
	if(serialBuffer[1]=='G' && serialBuffer[2]=='t') {
		currentCommand=Gt_GetLatitude;
	}
	if(serialBuffer[1]=='G' && serialBuffer[2]=='g') {
		currentCommand=Gg_GetLongitude;
	}
	if(serialBuffer[1]=='S' && serialBuffer[2]=='t') {
		currentCommand=St_SetLatitude;
	}
	if(serialBuffer[1]=='S' && serialBuffer[2]=='g') {
		currentCommand=Sg_SetLongitude;
	}
	if(serialBuffer[1]=='S' && serialBuffer[2]=='G') {
		currentCommand=SG_SetOffsetFromGMT;
	}

	int findCharPos;
	int startPos;
	int miscInt1;
	int miscInt2;
	switch(currentCommand) {
		case ACK:
			SerialSend("G#"); // respond to ack with mount type (G=GEM)
			break;
		case P_HighPrecisionToggle:
			highPrecision=!highPrecision;
			if(highPrecision) {
				progmemPrint(PSTR("HIGH PRECISION#"));
			} else {
				progmemPrint(PSTR("LOW PRECISION#"));
			}
			break;
		case U_ToggleLongFormat:
			longFormat=!longFormat;
			// returns nothing
			break;
		case CM_SyncToCurrentObject:
			mount -> sync(currentObjectPos);
			// progmemPrint(PSTR("0"));  // returns object name // TODO what is really normally returned?
			break;
		case MS_SlewToCurrentObject:
			mount -> gotoPos(currentObjectPos);
			progmemPrint(PSTR("0#")); // returns can slew // TODO implement impossible slew calcs when RTC arrives
			break;
		case RG_MotionRateGuide:
			motionRate = 10;
			break;
		case RC_MotionRateCenter:
			motionRate = 100;
			break;
		case RM_MotionRateFind:
			motionRate = 150;
			break;
		case RS_MotionRateSlew:
			motionRate = 255;
			break;
		case Mn_MoveNorth:
			mount -> slewNorth(motionRate);
			break;
		case Ms_MoveSouth:
		    mount -> slewSouth(motionRate);
			break;
		case Me_MoveEast:
			mount -> slewEast(motionRate);
			break;
		case Mw_MoveWest:
			mount -> slewWest(motionRate);
			break;
		case Qn_StopMoveNorth:
			mount -> stopSlew();
			break; 
		case Qs_StopMoveSouth:
			mount -> stopSlew();
			break;
		case Qe_StopMoveEast: 
			mount -> stopSlew();
			break;
		case Qw_StopMoveWest:
			mount -> stopSlew();
			break;
		case Q_AbortSlew:
			mount -> stopSlew();
			break;
		case c_Get12or24HourClockStatus:
			if(twentyFour_hourmode) {
				progmemPrint(PSTR("24#"));
			} else {
				progmemPrint(PSTR("12#"));
			}
			break;
		case GR_GetRAOfMount:
			printLX200positionRA(mount -> ra_axis -> currentAngle());
			break;
		case GD_GetDECOfMount:
			printLX200positionDEC(mount -> dec_axis -> currentAngle());
			break;
		case Sd_SetDECOfCurrentObject:
			if(setDEC()) {
				progmemPrint(PSTR("1#"));
			} else {
				progmemPrint(PSTR("0#"));
			}
			break;
		case Sr_SetRAOfCurrentObject:
			if(setRA()) {
				progmemPrint(PSTR("1#"));
			} else {
				progmemPrint(PSTR("0#"));
			}
			break;
		case GS_GetSiderealTime: // returns HH:MM:SS#
			break;
		case SS_SetSiderealTime: // :SS HH:MM:SS# returns Ok
			break;
		case GL_GetLocalTime: // Returns HH:MM:SS# in 24 hour format
			SerialSend(String(nowTime.hour()));
			progmemPrint(PSTR(":"));
			SerialSend(String(nowTime.minute()));
			progmemPrint(PSTR(":"));
			SerialSend(String(nowTime.second()));
			progmemPrint(PSTR("#"));
			break;
		case Ga_GetLocalTime: // Returns HH:MM:SS# in 12 hour format
			SerialSend(String(nowTime.hour12()));
			progmemPrint(PSTR(":"));
			SerialSend(String(nowTime.minute()));
			progmemPrint(PSTR(":"));
			SerialSend(String(nowTime.second()));
			progmemPrint(PSTR("#"));
			break;
		case SL_SetLocalTime: // :SL HH:MM:SS#  NOTE: The parameter should always be in 24 hour format.// returns Ok
			if(setLocalTime()) {
				progmemPrint(PSTR("1#"));
			} else {
				progmemPrint(PSTR("0#"));
			}
			break;
		case GC_GetCalendarDate: // Returns MM/DD/YY#
			SerialSend(String(nowTime.month()));
			progmemPrint(PSTR("/"));
			SerialSend(String(nowTime.day()));
			progmemPrint(PSTR("/"));
			SerialSend(String(nowTime.year()));
			progmemPrint(PSTR("#"));
			break;
		case SC_SetCalendarDate: //  :SC MM/DD/YY# returns Ok
			if(setDate()) {
				progmemPrint(PSTR("1#"));
			} else {
				progmemPrint(PSTR("0#"));
			}
			break;
		case St_SetLatitude: // :St+51*43
			progmemPrint(PSTR("1#")); // TODO
			break;
		case Sg_SetLongitude: // :Sg000*17
			progmemPrint(PSTR("1#")); // TODO
			break;
		case Gt_GetLatitude: 
			progmemPrint(PSTR("+51*43")); // TODO
			break;
		case Gg_GetLongitude:
			progmemPrint(PSTR("359*71")); // TODO
			break;
		case SG_SetOffsetFromGMT: // :SG+00
			progmemPrint(PSTR("1#")); // TODO
			break;
		default:
			// LX200 does not return any data when encoutering an unknown command
			break;
	}
}
