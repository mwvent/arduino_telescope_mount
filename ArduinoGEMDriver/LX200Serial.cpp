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

signed long LX200SerialHandler_c::getExtendedCommandValue() {
    int valuestartpos = findCharacterAndChangeToStringTerminator(&serialBuffer[0], 1, ' ');
    if(valuestartpos == -1) {
	    return -1;
    }
    return (signed long)atol(&serialBuffer[valuestartpos + 1]);
}

int LX200SerialHandler_c::findCharacter(char* chars, int startPos, char charToFind) {
	for(int findCharPos=startPos; true; findCharPos++) {
		if((chars[findCharPos]=='#' && charToFind!='#') || chars[findCharPos] == 0) { // found # or string terminator
			return -1; // return invalid
		}
		if(chars[findCharPos] == charToFind) { // found
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
			serialBuffer[serialBuffer_count] = 0; // swap the hash for a string terminator
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
			if(serialBuffer_count<49) {
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
			serialBuffer[serialBuffer_count] = 0; // swap the hash for a string terminator
			if(serialBuffer_count!=0) { // process command unless the hash was the only thing received
				serialCommand();
			}
			serialBuffer_count=0;
		} else {
			if(serialBuffer_count<49) {
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
void LX200SerialHandler_c::serialCommand() {
	DateTime nowTime = DS1307_RTC->getTime();
	int loopInt = 0;
	
	int findCharPos;
	int startPos;
	int miscInt1;
	int miscInt2;
	bool returnval;
	signed long miscSignedLong;
	
	// Misc commands
	if(serialBuffer[1]=='A'&&serialBuffer[2]=='C'&&serialBuffer[3]=='K') {
		SerialSend("G#"); // respond to ack with mount type (G=GEM)
	}
	if(serialBuffer[1]=='P') { // P_HighPrecisionToggle;
		highPrecision=!highPrecision;
		if(highPrecision) {
			progmemPrint(PSTR("HIGH PRECISION#"));
		} else {
			progmemPrint(PSTR("LOW PRECISION#"));
		}
	}
	if(serialBuffer[1]=='U') { // U_ToggleLongFormat
		longFormat=!longFormat;
		// returns nothing
	}
	if(serialBuffer[1]=='C' && serialBuffer[2]=='M') { // sync
		mount -> sync(currentObjectPos);
		// progmemPrint(PSTR("0"));  // returns object name according to docs but causes problems with client software - TODO what is really normally returned?
	}

	switch(serialBuffer[1]) {
		// Set (S) commands
		case 'S' :
			switch(serialBuffer[2]) {
				case 'r': // Sr_SetRAOfCurrentObject
					if(setRA()) {
						progmemPrint(PSTR("1#"));
					} else {
						progmemPrint(PSTR("0#"));
					}
					break;
				case 'S': // SS_SetSiderealTime
					// TODO
					break;
				case 'C': // SC_SetCalendarDate
					if(setDate()) {
						progmemPrint(PSTR("1#"));
					} else {
						progmemPrint(PSTR("0#"));
					}
				case 'L': // SL_SetLocalTime
					if(setLocalTime()) {
						progmemPrint(PSTR("1#"));
					} else {
						progmemPrint(PSTR("0#"));
					}
					break;
				case 'd': // Sd_SetDECOfCurrentObject
					if(setDEC()) {
						progmemPrint(PSTR("1#"));
					} else {
						progmemPrint(PSTR("0#"));
					}
					break;
				case 't': // St_SetLatitude
					progmemPrint(PSTR("1#")); // TODO
					break;
				case 'g': // Sg_SetLongitude
					progmemPrint(PSTR("1#")); // TODO
					break;
				case 'G': // SG_SetOffsetFromGMT
					progmemPrint(PSTR("1#")); // TODO
					break;
			}
			break;
		
		// Get (G) commands
		case 'G' :
			switch(serialBuffer[2]) {
				case 'R' : // GR_GetRAOfMount;
					printLX200positionRA(mount -> ra_axis -> currentAngle());
					break;
				case 'D' : // GD_GetDECOfMount
					printLX200positionDEC(mount -> dec_axis -> currentAngle());
					break;
				case 'S' : // GS_GetSiderealTime
					// TODO
					break;
				case 'L' :  // GL_GetLocalTime
					SerialSend(String(nowTime.hour()));
					progmemPrint(PSTR(":"));
					SerialSend(String(nowTime.minute()));
					progmemPrint(PSTR(":"));
					SerialSend(String(nowTime.second()));
					progmemPrint(PSTR("#"));
					break;
				case 'a' : // Ga_GetLocalTime
					SerialSend(String(nowTime.hour12()));
					progmemPrint(PSTR(":"));
					SerialSend(String(nowTime.minute()));
					progmemPrint(PSTR(":"));
					SerialSend(String(nowTime.second()));
					progmemPrint(PSTR("#"));
					break;
				case 'C' : // GC_GetCalendarDate
					SerialSend(String(nowTime.month()));
					progmemPrint(PSTR("/"));
					SerialSend(String(nowTime.day()));
					progmemPrint(PSTR("/"));
					SerialSend(String(nowTime.year()));
					progmemPrint(PSTR("#"));
					break;
				case 'c' : // Gc_Get12or24HourClockStatus
					if(twentyFour_hourmode) {
						progmemPrint(PSTR("24#"));
					} else {
						progmemPrint(PSTR("12#"));
					}
					break;
				case 't' : // Gt_GetLatitude
					progmemPrint(PSTR("+51*43")); // TODO
					break;
				case 'g' : // Gg_GetLongitude
					progmemPrint(PSTR("359*71")); // TODO
					break;
			}
			break;
			
		// Movement (M) commands
		case 'M' :
			switch(serialBuffer[2]) {
				case 'S' : // MS_SlewToCurrentObject
					mount -> gotoPos(currentObjectPos);
					// returns can slew.. always! - could really use some checking to avoid mount crashes!
					progmemPrint(PSTR("0#")); 
					break;
				case 'n' : // Mn_MoveNorth
					mount -> slewNorth(motionRate);
					break;
				case 's' : // Ms_MoveSouth
					mount -> slewSouth(motionRate);
					break;
				case 'e' : // Me_MoveEast
					mount -> slewEast(motionRate);
					break;
				case 'w' : // Mw_MoveWest
					mount -> slewWest(motionRate);
					break;
			}
			break;
		
		// Movement stop (Q) commands
		case 'Q' :
			/* No need in current implemtation to check second char - all paths lead to same end
			switch(serialBuffer[2]) {
				case 'n' : // Qn_StopMoveNorth
					mount -> stopSlew();
					break;
				case 's' : // Qs_StopMoveSouth
					mount -> stopSlew();
					break;
				case 'e' : // Qe_StopMoveEast
					mount -> stopSlew();
					break;
				case 'w' : // Qw_StopMoveWest
					mount -> stopSlew();
					break;
				case 0 : // Q_stopslew
					mount -> stopSlew();
					break;
			}
			*/
			mount -> stopSlew();
			break;
		
		// Rate set (R) commands
		case 'R' :
			switch(serialBuffer[2]) {
				case 'G' : // RG_MotionRateGuide
					motionRate = 10;
					break;
				case 'C' : // RC_MotionRateCenter
					motionRate = 100;
					break;
				case 'M' : // RM_MotionRateFind
					motionRate = 150;
					break;
				case 'S' : // RS_MotionRateSlew
					motionRate = 255;
					break;
			}
			break;
		// Extended (E) commands - not part of the LX200 protocol
		case 'E' :
			miscSignedLong = getExtendedCommandValue(); // check if there was a value given after command
			switch(serialBuffer[2]) {
				// value read/set commands - if a value was supplied after the command
			        // (seperated by a space) then it sets the value
				// the value is read back in either case
				case 'X' : // Get gearpos_max for RA axis - set not availible
					SerialSend(String(mount -> ra_axis -> gearPosition_max));
					SerialSend("#");
					break; 
				case 'Y' : // Get gearpos_max for DEC axis - set not availible
					SerialSend(String(mount -> dec_axis -> gearPosition_max));
					SerialSend("#");
					break;
				case 'R' : // Get gearpos for RA axis
					if(miscSignedLong != -1) {
					  mount -> ra_axis -> gearPosition_raw = miscSignedLong;
					}
					SerialSend(String(mount -> ra_axis -> gearPosition_raw));
					SerialSend("#");
					break;
				case 'D' : // Get gearpos for DEC axis
					if(miscSignedLong != -1) {
					  mount -> dec_axis -> gearPosition_raw = miscSignedLong;
					}
					SerialSend(String(mount -> dec_axis -> gearPosition_raw));
					SerialSend("#");
					break;
				case 'r' : // Get gearpos_target for RA axis
					if(miscSignedLong != -1) {
					  mount -> ra_axis -> gearPosition_target = miscSignedLong;
					}
					SerialSend(String(mount -> ra_axis -> gearPosition_target));
					SerialSend("#");
					break;
				case 'd' : // Get gearpos_target for DEC axis
					if(miscSignedLong != -1) {
					  mount -> dec_axis -> gearPosition_target = miscSignedLong;
					}
					SerialSend(String(mount -> dec_axis -> gearPosition_target));
					SerialSend("#");
					break;
			}
			break;
	}
}
