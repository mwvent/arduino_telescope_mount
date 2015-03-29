#include <sys/time.h>
#include <math.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "indi_arduino_gem_driver.h"
#include "indicom.h"
const int   POLLMS	 =	250;    			/* poll period, ms */
std::auto_ptr<ArduinoGEMDriver> arduinoGEMDriver(0);
/**************************************************************************************
** Initilize ArduinoGEMDriver object
***************************************************************************************/
void ISInit() {
	// prevent re-Initilization
	static int isInit=0;
	if (isInit) {
		return;
	}
	if (arduinoGEMDriver.get() == 0) {
		isInit = 1;
		arduinoGEMDriver.reset(new ArduinoGEMDriver());
	}
}
/**************************************************************************************
** Return properties of device.
***************************************************************************************/
void ISGetProperties (const char *dev) {
	ISInit();
	arduinoGEMDriver->ISGetProperties(dev);
}
/**************************************************************************************
** Process new switch from client
***************************************************************************************/
void ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n) {
	ISInit();
	arduinoGEMDriver->ISNewSwitch(dev, name, states, names, n);
}
/**************************************************************************************
** Process new text from client
***************************************************************************************/
void ISNewText (const char *dev, const char *name, char *texts[], char *names[], int n) {
	ISInit();
	arduinoGEMDriver->ISNewText(dev, name, texts, names, n);
}
/**************************************************************************************
** Process new number from client
***************************************************************************************/
void ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n) {
	ISInit();
	arduinoGEMDriver->ISNewNumber(dev, name, values, names, n);
}
/**************************************************************************************
** Process new blob from client
***************************************************************************************/
void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n) {
	ISInit();
	arduinoGEMDriver->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}
/**************************************************************************************
** Process snooped property from another driver
***************************************************************************************/
void ISSnoopDevice (XMLEle *root) {
	INDI_UNUSED(root);
}
/**************************************************************************************
** Main Class
***************************************************************************************/
// Class constuctor
ArduinoGEMDriver::ArduinoGEMDriver() {
	// We add an additional debug level so we can log verbose scope status
	DBG_SCOPE = INDI::Logger::getInstance().addDebugLevel("Scope Verbose", "SCOPE");
	TelescopeCapability cap;
	cap.canPark = false;
	cap.canSync = true;
	cap.canAbort = true;
	SetTelescopeCapability(&cap);
}

//  INDI initProperties
bool ArduinoGEMDriver::initProperties() {
	// ALWAYS call initProperties() of parent first
	INDI::Telescope::initProperties();
	addDebugControl();
	return true;
}

//  INDI called connect
bool ArduinoGEMDriver::Connect() {
	bool rc=false;
	if(isConnected()) return true;
	if(Connect(PortT[0].text)) {
		SetTimer(POLLMS);
		return true;
	}
	return false;
}

//  Open and ready serial connection to the controller
bool ArduinoGEMDriver::Connect(const char *port) {
	int connectrc=0;
	char errorMsg[MAXRBUF];
	bool rc;
	if ( (connectrc = tty_connect(port, 9600, 8, 0, 1, &PortFD)) != TTY_OK) {
		tty_error_msg(connectrc, errorMsg, MAXRBUF);
		return false;
	}
	flushSerialBuffer(4); // flush any unwanted startup data from serial port
	// Test connection
	if(ACK() == true) {
		//  If the ACK is received go ahead and get the static properties from the device
		return get_static_properties();
	}
	//  ACK failed
	tty_disconnect(PortFD);
	return false;
}

//  INDI called disconnect
bool ArduinoGEMDriver::Disconnect() {
	tty_disconnect(PortFD);
	return true;
}

//  Tell indi the driver name
const char * ArduinoGEMDriver::getDefaultName() {
	return "Arduino GEM Driver";
}

//  Move telescope to a new position by setting the controllers target position
bool ArduinoGEMDriver::Goto(double ra, double dec) {
	int newRAGearpos = (int) ((ra / 24.0) * (double)RA_gearpos_max);
	int newDECGearpos = (int) ((DECToAxisDegrees(dec) / 360.0) * (double)DEC_gearpos_max);
	char newRAGearpos_target_command[100];
	char newDECGearpos_target_command[100];

	snprintf(newRAGearpos_target_command, 100, ":Er %i#", newRAGearpos);
	snprintf(newDECGearpos_target_command, 100, ":Ed %i#", newDECGearpos);
	
	char ra_tar_response[100];
	char dec_tar_response[100];
	
 	if(! serialCommand(newRAGearpos_target_command, ra_tar_response, true) ) { return false; }
 	if(! serialCommand(newDECGearpos_target_command, dec_tar_response, true) ) { return false; }
	
	//  Get numbers
	RA_gearpos_target = atoi(ra_tar_response);
	DEC_gearpos_target  = atoi(dec_tar_response);
	
	IDLog("Read new target %d, %d\n", RA_gearpos_target, DEC_gearpos_target);
	
	return true;
}

//  Sync the telescope to a new position by setting the controllers position and target position to the same new value
bool ArduinoGEMDriver::Sync(double ra, double dec) {
	int newRAGearpos = (int) ((ra / 24.0) * (double)RA_gearpos_max);
	int newDECGearpos = (int) ((DECToAxisDegrees(dec) / 360.0) * (double)DEC_gearpos_max);
	char newRAGearpos_command[100];
	char newRAGearpos_target_command[100];
	char newDECGearpos_command[100];
	char newDECGearpos_target_command[100];

	snprintf(newRAGearpos_command, 25, ":ER %i#", newRAGearpos);
	snprintf(newRAGearpos_target_command, 25, ":Er %i#", newRAGearpos);
	snprintf(newDECGearpos_command, 25, ":ED %i#", newDECGearpos);
	snprintf(newDECGearpos_target_command, 25, ":Ed %i#", newDECGearpos);
	
	char ra_response[100];
	char dec_response[100];
	char ra_tar_response[100];
	char dec_tar_response[100];
	
 	if(! serialCommand(newRAGearpos_command, ra_response, true) ) { return false; }
 	if(! serialCommand(newRAGearpos_target_command, ra_tar_response, true) ) { return false; }
 	if(! serialCommand(newDECGearpos_command, dec_response, true) ) { return false; }
 	if(! serialCommand(newDECGearpos_target_command, dec_tar_response, true) ) { return false; }
 	
	//  Get numbers
	RA_gearpos = atoi(ra_response);
	DEC_gearpos  = atoi(dec_response);
	RA_gearpos_target = atoi(ra_tar_response);
	DEC_gearpos_target  = atoi(dec_tar_response);
	
	//  Calculate position
	double newRA = ( (double)RA_gearpos / (double)RA_gearpos_max ) * 24.0;
	double newDEC = AxisDegreesToDec(( (double)DEC_gearpos / (double)DEC_gearpos_max ) * 360.0);
	NewRaDec(newRA, newDEC);
	IDLog("Read new position %d, %d, %d, %d as RA%f, DEC%fn", RA_gearpos, DEC_gearpos, RA_gearpos_target, DEC_gearpos_target, newRA, newDEC);
	
	return true;
}

//  Abort motion
bool ArduinoGEMDriver::Abort() {
	char command[100];
	snprintf(command, 100, ":Q#");
	return serialCommand(command, command, false);
}

//  Pulse guiding commands
bool ArduinoGEMDriver::GuideNorth(float ms) {
	char command[100];
	int roundms = (int)ms;
	snprintf(command, 100, ":EN %i#", roundms);
	return serialCommand(command, command, false);
}
bool ArduinoGEMDriver::GuideSouth(float ms) {
	char command[100];
	int roundms = (int)ms;
	snprintf(command, 100, ":ES %i#", roundms);
	return serialCommand(command, command, false);
}
bool ArduinoGEMDriver::GuideEast(float ms) {
	char command[100];
	int roundms = (int)ms;
	snprintf(command, 100, ":EE %i#", roundms);
	return serialCommand(command, command, false);
}
bool ArduinoGEMDriver::GuideWest(float ms) {
	char command[100];
	int roundms = (int)ms;
	snprintf(command, 100, ":EW %i#", roundms);
	return serialCommand(command, command, false);
}

//  Move a direction on the DEC axis
bool ArduinoGEMDriver::MoveNS(TelescopeMotionNS dir, TelescopeMotionCommand command)
{
	char command[100];
	switch (command)
	{
		case MOTION_START:
			if (dir == MOTION_NORTH) {
				snprintf(command, 100, ":Mn#");
				return serialCommand(command, command, false);
			} else {
				snprintf(command, 100, ":Ms#");
				return serialCommand(command, command, false);
			}
			break;
		case MOTION_STOP:
			return Abort();
			break;
	}
	return true;
}

//  Move a direction on the RA axis
bool ArduinoGEMDriver::MoveWE(TelescopeMotionWE dir, TelescopeMotionCommand command)
{
	char command[100];
	switch (command)
	{
		case MOTION_START:
			if (dir == MOTION_WEST) {
				snprintf(command, 100, ":Mw#");
				return serialCommand(command, command, false);
			} else {
				snprintf(command, 100, ":Me#");
				return serialCommand(command, command, false);
			}
			break;
		case MOTION_STOP:
			return Abort();
			break;
	}
	return true;
}

// Read positional values from controller (for polling)
bool ArduinoGEMDriver::ReadScopeStatus() {
	char command[100];
	char ra_response[100];
	char dec_response[100];
	char ra_tar_response[100];
	char dec_tar_response[100];
	
	strcpy(command, ":ER#\0");
 	if(! serialCommand(command, ra_response, true) ) { return false; }
	RA_gearpos = atoi(ra_response);
	strcpy(command, ":Er#\0");
 	if(! serialCommand(command, ra_tar_response, true) ) { return false; }
	RA_gearpos_target = atoi(ra_tar_response);
	strcpy(command, ":ED#\0");
 	if(! serialCommand(command, dec_response, true) ) { return false; }
	DEC_gearpos  = atoi(dec_response);
	strcpy(command, ":Ed#\0");
 	if(! serialCommand(command, dec_tar_response, true) ) { return false; }
	DEC_gearpos_target  = atoi(dec_tar_response);
	
	double newRA = ( (double)RA_gearpos / (double)RA_gearpos_max ) * 24.0;
	double newDEC = AxisDegreesToDec(( (double)DEC_gearpos / (double)DEC_gearpos_max ) * 360.0);
	NewRaDec(newRA, newDEC);
	
	IDLog("Read new position %d, %d, %d, %d as RA%f, DEC%fn", RA_gearpos, DEC_gearpos, RA_gearpos_target, DEC_gearpos_target, newRA, newDEC);
	
	// set scope state
	int RA_Diff = abs(RA_gearpos_target - RA_gearpos);
	int DEC_Diff = abs(DEC_gearpos_target - DEC_gearpos);
	//  if the diff is large enougth we are slewing - if small we are tracking
	if ( (RA_Diff > 50) || (DEC_Diff > 50) ) {
		TrackState=SCOPE_SLEWING;
	} else {
		TrackState=SCOPE_TRACKING;
	}
	
	return true;
}

// Send an ACK command to the server and check for correct response
char ArduinoGEMDriver::ACK() {
	char response[100];
	char command[100];
	strcpy(command, ":ACK#\0");
 	if(! serialCommand(command, response, true) ) { return false; }
	
	if(response[0] == 'G') {
		IDLog("ACK received\n");
		return true;
	}
	
	IDLog("Out of sync with serial comms, expected G - received %s\n", response);
	return false;
}

// Retreive any non-changing or static values from the controller
bool ArduinoGEMDriver::get_static_properties() {
	char ra_response[100];
	char dec_response[100];
	char command[100];
	
	strcpy(command, ":EX#\0");
 	if(! serialCommand(command, ra_response, true) ) { return false; }
	strcpy(command, ":EY#\0");
 	if(! serialCommand(command, dec_response, true) ) { return false; }
	RA_gearpos_max = atoi(ra_response);
	IDLog("RA_gearpos_max=%d\n", RA_gearpos_max);
	DEC_gearpos_max  = atoi(dec_response);
	IDLog("DEC_gearpos_max=%d\n", DEC_gearpos_max);
}

// get rid of any incoming serial data (for getting in sync with the protocol)
void ArduinoGEMDriver::flushSerialBuffer(int timeout) {
	char flushbuffer[400];
	int nbytes_read;
	int flushresponse = tty_read(PortFD, flushbuffer, 400, timeout, &nbytes_read);
}

// take a dec value of -90 to 90 and convert to a 360 degree angle for the physical mount axis
double ArduinoGEMDriver::DECToAxisDegrees(double dec)  {
	
	// add 90 to dec so we get a half-circle range of 0 to 180 instead of -90 to +90
	double dec_halfcircle = dec + 90.0;
	// the next half of the circle has rangle +90 to -90 so if the current half is the other side is inverse
	// we check which half the actual axis is currentley pointed and if it is on the inverse half then adjust the
	// value
	ReadScopeStatus();
	double currentAxisDegrees = ( (double)DEC_gearpos / (double)DEC_gearpos_max ) * 360.0;
	bool otherHalf = ( currentAxisDegrees >= 180.0 );
	if(otherHalf) { dec_halfcircle = 180.0 + (180.0 - dec_halfcircle); }
	// done ! 0-360
	return dec_halfcircle;
}

// take a degree value of 0-360 and work out the dec value
double ArduinoGEMDriver::AxisDegreesToDec(double degrees) {
	// if degress is over 180 we are one the inverse side so take away 180 then invert the remainder
	// so 225 = 180-(225-180)=135   270degrees = 180-(270-180) = 90    315 = 180-(315-180) = 45
	bool otherHalf = ( degrees >= 180.0 );
	double half_circle_degress = 180.0 - (degrees - 180.0);
	// we now have a half_circle_degress value from 0-180 - takeaway 90 to get to a range of -90 to +90
	half_circle_degress = half_circle_degress - 90.0;
	// done ! -90 to +90
	return half_circle_degress;
}

bool ArduinoGEMDriver::serialCommand(char *command, char *responseBuffer, bool getResponse) {
	int nbytes_write=0, nbytes_read=0, error_type, err_msg_len;
	char err_msg[200];
	error_type = tty_write_string(PortFD, command, &nbytes_write);
	if( error_type != TTY_OK) {
		tty_error_msg(error_type, err_msg, err_msg_len);
		err_msg[err_msg_len] = '\0';
		IDLog("Serial Error sending %s - %s \n", command, err_msg);
		return false;
	}
	if(!getResponse) { return true; }
	error_type = tty_read_section(PortFD, responseBuffer, '#', 2, &nbytes_read);
	if(error_type != TTY_OK) {
		tty_error_msg(error_type, err_msg, err_msg_len);
		err_msg[err_msg_len] = '\0';
		IDLog("Serial Error receiving response to %s - %s \n", command, err_msg);
		return false;
	}
	if(nbytes_read == 0) {
		IDLog("No reponse to command %s from controller\n", command);
		return false;
	}
	responseBuffer[nbytes_read] = '\0'; // add string terminator
	return true;
}
