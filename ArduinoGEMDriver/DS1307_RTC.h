// Handles reading and writing to the DS1307 real-time clock
// giving access to the clock bits as a DateTime object and
// also giving access to the general pourpose nvram

#ifndef _DS1307_RTC_h_
#define _DS1307_RTC_h_
#include "settings.h"
#include "DateTime.h"
#include "Wire.h"
#include "types.h"

#define DS1307_ADDRESS 0x68

class DS1307_RTC_c
{
	public:
		DS1307_RTC_c();
		String TimeString();
		String DateString();
		DateTime getTime();
		void setTime(const DateTime& dt);
		long getRAgearpos();
		void setRAgearpos(long newPosition);
		long getDECgearpos();
		void setDECgearpos(long newPosition);
		location getSiteLocation();
		void setSiteLocation(location newLocation);
		int readInt(byte pos);
		void writeInt(byte pos, int value);
		byte readByte(byte pos);
		void blockRead(byte startpos, byte amountToRead, byte target[]);
		void writeByte(byte pos, byte value);
		unsigned long readUnsignedLong(byte pos);
		void writeUnsignedLong(byte pos, unsigned long value);
		signed long readSignedLong(byte pos);
		void writeSignedLong(byte pos, signed long value);
		void readRTC(); // read RTC portion of DS1307 ram only
	private:
		DateTime* lastTimeRead;
		static uint8_t bcd2bin (uint8_t val);
		static uint8_t bin2bcd (uint8_t val);
};

#endif
