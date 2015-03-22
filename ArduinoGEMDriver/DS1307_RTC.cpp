// Handles reading and writing to the DS1307 real-time clock
// giving access to the clock bits as a DateTime object (see DateTime.h ) and
// also giving access to the general pourpose nvram

#include "DS1307_RTC.h"

DS1307_RTC_c::DS1307_RTC_c() {
	// Wire.begin();
	lastTimeRead = new DateTime(2000, 01, 01, 01, 00, 00);
}

String DS1307_RTC_c::TimeString() {
	String TimeOutBuf;
	DateTime now = getTime();
	TimeOutBuf = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
	return TimeOutBuf;
}

String DS1307_RTC_c::DateString() {

}

/////////////////////////
// Get & Set Time
/////////////////////////

void DS1307_RTC_c::readRTC() {
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(0); // pos 0
	Wire.endTransmission();
	Wire.requestFrom(DS1307_ADDRESS, 7); // request next byte
	unsigned long timeout = millis() + 200;
	while(Wire.available()<7&millis()<timeout) {
		delay(1);    // wait for next byte to become availible
	}
	if(Wire.available()<7) {
		#ifdef debugOut_I2CTimeouts
		  Serial.println("DS1037 Timeout");
		#endif
		return;
	} else {
		#ifdef debugOut_I2CTimeouts
		  // Serial.println("DS1037 Time Read OK"); // causes reboot so commented out - weird
		#endif
	}
	uint8_t ss = bcd2bin(Wire.read() & 0x7F);
	uint8_t mm = bcd2bin(Wire.read());
	uint8_t hh = bcd2bin(Wire.read());
	uint8_t nout = bcd2bin(Wire.read());
	uint8_t d = bcd2bin(Wire.read());
	uint8_t m = bcd2bin(Wire.read());
	uint16_t y = bcd2bin(Wire.read()) + 2000;
	delete lastTimeRead;
	lastTimeRead = new DateTime (y, m, d, hh, mm, ss);
}

DateTime DS1307_RTC_c::getTime() {
	// pull in data from DS1307 no quicker than once per second
	static unsigned long lastRTCupdate = millis();
	if(millis() > (lastRTCupdate+1000)) {
		lastRTCupdate = millis();
		readRTC();
	}
	return *lastTimeRead;
}

void DS1307_RTC_c::setTime(const DateTime& dt) {
	byte retryCount = 0;
	bool done = false;
	while((retryCount < 5) && (!done)) {
		// save changes to buffer
		Wire.beginTransmission(DS1307_ADDRESS);
		Wire.write(0); // position 0
		Wire.write(bin2bcd(dt.second()));
		Wire.write(bin2bcd(dt.minute()));
		Wire.write(bin2bcd(dt.hour()));
		Wire.write(0);
		Wire.write(bin2bcd(dt.day()));
		Wire.write(bin2bcd(dt.month()));
		Wire.write(bin2bcd(dt.year() - 2000));
		done = (Wire.endTransmission()==0);
		retryCount++;
	}
	// force refresh if set OK
	if(done) {
		readRTC();
	}
}

////////////////////////////////////////////////////////////
// Binary coded decimal translation for RTC time (en/de)code
////////////////////////////////////////////////////////////
uint8_t DS1307_RTC_c::bcd2bin (uint8_t val) {
	return val - 6 * (val >> 4);    // copied from RTCLib
}
uint8_t DS1307_RTC_c::bin2bcd (uint8_t val) {
	return val + 6 * (val / 10);    // copied from RTCLib
}


////////////////////////////////////////////////////////////////////////////////////
// Access NVRAM uses 50 bytes of GP RAM from pos 13-63
// Position is multiped by two so this is treated as an array of 25 signed ints
////////////////////////////////////////////////////////////////////////////////////

void DS1307_RTC_c::blockRead(byte startpos, byte amountToRead, byte target[]) {
	if(startpos > 50) {
		return;
	}
	if(startpos+amountToRead > 50) {
		return;
	}
	startpos += 13;
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(startpos); // address
	Wire.endTransmission();
	Wire.requestFrom((int)DS1307_ADDRESS, (int)amountToRead); // request 2 bytes for int
	unsigned long timeout;
	byte amountLeft = amountToRead;
	byte currentPos = 0;
	byte currentVal = 0;
	while(amountLeft > 0) {
		timeout = millis() + 100;
		while( (millis()<timeout) & (Wire.available()<amountToRead) ) {
			delay(1);    // wait for I2C data or timeout
		}
		if(Wire.available() == 0) {
			return;
		}
		currentVal = Wire.read();
		target[currentPos] = currentVal;
		currentPos++;
		amountToRead--;
	}
}

signed int DS1307_RTC_c::readInt(byte pos) {
	if(pos > 25) {
		return 0;
	}
	pos=pos+pos+13;
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(pos); // address
	Wire.endTransmission();
	Wire.requestFrom(DS1307_ADDRESS, 2); // request 2 bytes for int
	unsigned long timeout = millis() + 100; // set max amount of time before giving up on read
	while( (millis()<timeout) & (Wire.available()<2) ) {
		delay(1);    // wait for I2C data or timeout
	}
	if(Wire.available()>=2) {
		byte MSB=Wire.read();
		byte LSB=Wire.read();
		return (((signed int)MSB) << 8) | ((signed int)LSB) ;
	} else {
		return 0;
	}
}

void DS1307_RTC_c::writeInt(byte pos, signed int value) {
	if(pos > 25) {
		return;
	}
	pos=pos+pos+13;
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(pos); // address
	Wire.write((int)(value >> 8)); // msb of data
	Wire.write((int)(value & 0x00FF)); // lsb of data
	Wire.endTransmission();
}

byte DS1307_RTC_c::readByte(byte pos) {
	if(pos > 50) {
		return 0;
	}
	pos=pos+13;
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(pos); // address
	Wire.endTransmission();
	Wire.requestFrom(DS1307_ADDRESS, 1); // request 1 byte
	unsigned long timeout = millis() + 100; // set max amount of time before giving up on read
	while( (millis()<timeout) & (Wire.available()==0) ) {
		delay(1);    // wait for I2C data or timeout
	}
	if(Wire.available()>=1) {
		return Wire.read();
	} else {
		return 0;
	}
}

void DS1307_RTC_c::writeByte(byte pos, byte value) {
	if(pos > 50) {
		return;
	}
	pos=pos+13; // offset past clock bits for general p ram
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(pos); // address
	Wire.write(value);
	Wire.endTransmission();
}
