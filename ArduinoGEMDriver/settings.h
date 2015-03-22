// A lot of programs that connect to the serial port to talk LX200 send an ACK immediatley while the bootloader is 
// still running - the define ensures the G# reply to the ACK command is sent at startup to fix the issue
#define _LX200_TIMEOUT_FIX
// -------------------------------------------------------------
// Compile-Time mount setup
// -------------------------------------------------------------
#define _RA_ENCODER_PPR 4096
#define _DEC_ENCODER_PPR 8400
#define _RA_ENCODER_GEAR_TEETH 138
#define _DEC_ENCODER_GEAR_TEETH 88
#define _RA_MOTOR_PORT 1
#define _DEC_MOTOR_PORT 2
#define _FOCUS_MOTOR_PORT 3

// -------------------------------------------------------------
// Defaults for user adjustable values
// -------------------------------------------------------------
#define _default_RASlowSpeed 20
#define _default_DECSlowSpeed 80
#define _default_RAErrorTolerance 3
#define _default_DECErrorTolerance 3

// --------------------------------------------------------------------------------------------
// USE The DS1307 for Settings storage, more flexible then EEPROM as can be written frequentley
// This will enable dynamic storage of the motor calibration and axis positions in a future release
// --------------------------------------------------------------------------------------------
#define _USE_DS1307_NVRAM

// -------------------------------------------------------------
// Debug Flags - turn these off if you want to use SPI0 as an LX200 interface
// -------------------------------------------------------------
// #define debugOut_I2CTimeouts
// #define debugOut_shaftPositions
// #define debugOut_motorResponseTimeOuts
// #define debugOut_showSPI1commandsOnSPI0
// #define debugOut_startuplog

// -------------------------------------------------------------
// Motor
// -------------------------------------------------------------
#define _DEC_MOTOR_MAX_SPEED 120 //90
#define _RA_MOTOR_MAX_SPEED 255
// #define _RA_DIRECTION_REVERSE
// #define _DEC_DIRECTION_REVERSE
