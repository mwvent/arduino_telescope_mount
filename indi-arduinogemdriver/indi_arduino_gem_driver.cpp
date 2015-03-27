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
ArduinoGEMDriver::ArduinoGEMDriver() {
  // We add an additional debug level so we can log verbose scope status
  DBG_SCOPE = INDI::Logger::getInstance().addDebugLevel("Scope Verbose", "SCOPE");

  TelescopeCapability cap;
  cap.canPark = false;
  cap.canSync = true;
  cap.canAbort = true;
  SetTelescopeCapability(&cap);
}

bool ArduinoGEMDriver::initProperties() {
  // ALWAYS call initProperties() of parent first
  INDI::Telescope::initProperties();
  addDebugControl();
  return true;
}

bool ArduinoGEMDriver::Connect() {
  bool rc=false;
  if(isConnected()) return true;
  if(Connect(PortT[0].text)) {
    SetTimer(POLLMS);
    return true;
  }
  return false;
}

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
    if(ACK() == 0) {
        //  If the ACK is received go ahead and get the static properties from the device
        return get_static_properties();
    }
    //  ACK failed
    tty_disconnect(PortFD);
    return false;
}

bool ArduinoGEMDriver::Disconnect() {
    tty_disconnect(PortFD);
    return true;
}

const char * ArduinoGEMDriver::getDefaultName() {
    return "Arduino GEM Driver";
}

bool ArduinoGEMDriver::Goto(double ra, double dec) {
    return true;
}

bool ArduinoGEMDriver::Sync(double ra, double dec) {
  int newRAGearpos = (int) ((ra / 24.0) * (double)RA_gearpos_max);
  int newDECGearpos = (int) ((DECToAxisDegrees(dec) / 360.0) * (double)DEC_gearpos_max);
  char newRAGearpos_command[25];
  char newRAGearpos_target_command[25];
  char newDECGearpos_command[25];
  char newDECGearpos_target_command[25];
  char allCommands[100];
  
  snprintf(newRAGearpos_command, 25, ":ER %i#", newRAGearpos);
  snprintf(newRAGearpos_target_command, 25, ":Er %i#", newRAGearpos);
  snprintf(newDECGearpos_command, 25, ":ED %i#", newDECGearpos);
  snprintf(newDECGearpos_target_command, 25, ":Ed %i#", newDECGearpos);
  snprintf(allCommands, 100, "%s%s%s%s", newRAGearpos_command, newRAGearpos_target_command, newDECGearpos_command, newDECGearpos_target_command);
  
  int bytes_written;
  if(tty_write_string(PortFD, allCommands, &bytes_written) != TTY_OK) {
    IDLog("Failed to write sync commands to buffer '%s' from ra=%f dec=%f\n", allCommands, ra, dec);
    return false;
  } else {
    IDLog("Sent sync commands '%s' from ra=%f dec=%f\n", allCommands, ra, dec);
  }
  
  char ra_response[25];
  char dec_response[25];
  char ra_tar_response[25];
  char dec_tar_response[25];
  int nbytes_write=0, nbytes_read=0, error_type;
 
  error_type = tty_read_section(PortFD, ra_response, '#', 2, &nbytes_read);
  if(nbytes_read == 0) {
    IDLog("No response from mount when reading ER\n");
    return false;
  }
  ra_response[nbytes_read +1] = 0; // add string terminator
  
  error_type = tty_read_section(PortFD, ra_tar_response, '#', 2, &nbytes_read);
  if(nbytes_read == 0) {
    IDLog("No response from mount when reading Er\n");
    return false;
  }
  ra_tar_response[nbytes_read +1] = 0; // add string terminator
  
  error_type = tty_read_section(PortFD, dec_response, '#', 2, &nbytes_read);
  if(nbytes_read == 0) {
    IDLog("No response from mount when reading ED\n");
    return false;
  }
  dec_response[nbytes_read +1] = 0; // add string terminator
  
  error_type = tty_read_section(PortFD, dec_tar_response, '#', 2, &nbytes_read);
  if(nbytes_read == 0) {
    IDLog("No response from mount when reading Ed\n");
    return false;
  }
  dec_tar_response[nbytes_read +1] = 0; // add string terminator
  
  RA_gearpos = atoi(ra_response);
  DEC_gearpos  = atoi(dec_response);
  RA_gearpos_target = atoi(ra_tar_response);
  DEC_gearpos_target  = atoi(dec_tar_response);
  
  double newRA = ( (double)RA_gearpos / (double)RA_gearpos_max ) * 24.0;
  double newDEC = AxisDegreesToDec(( (double)DEC_gearpos / (double)DEC_gearpos_max ) * 360.0);
  
  NewRaDec(newRA, newDEC);
  IDLog("Read new position %d, %d, %d, %d as RA%f, DEC%f\n", RA_gearpos, DEC_gearpos, RA_gearpos_target, DEC_gearpos_target, newRA, newDEC);
  return true;
}

bool ArduinoGEMDriver::Abort() {
    return true;
}

bool ArduinoGEMDriver::GuideNorth(float ms) {
  return true;
}
bool ArduinoGEMDriver::GuideSouth(float ms) {
  return true;
}
bool ArduinoGEMDriver::GuideEast(float ms) {
  return true;
}
bool ArduinoGEMDriver::GuideWest(float ms) {
  return true;
}

bool ArduinoGEMDriver::MoveNS(TelescopeMotionNS dir, TelescopeMotionCommand command)
{
    switch (command)
    {
        case MOTION_START:
	  if (dir == MOTION_NORTH) {
	    // 
	  } else {
	    // 
	  }
	  break;
        case MOTION_STOP:
	  // 
	  break;
    }

    return true;
}

bool ArduinoGEMDriver::MoveWE(TelescopeMotionWE dir, TelescopeMotionCommand command)
{
    switch (command)
    {
        case MOTION_START:
	  if (dir == MOTION_WEST) {
	    // 
	  } else {
	    // 
	  }
	  break;
        case MOTION_STOP:
	  // 
	  break;
    }

    return true;
}

bool ArduinoGEMDriver::ReadScopeStatus() {
  char sendchars[17] = ":ER#:ED#:Er#:Ed#";
  char ra_response[15];
  char dec_response[15];
  char ra_tar_response[15];
  char dec_tar_response[15];
  int nbytes_write=0, nbytes_read=0, error_type;

  nbytes_write = write(PortFD, sendchars, 16);

  if (nbytes_write < 0) {
    IDLog("Could not write to serial port\n");
    return false;
  }
 
  error_type = tty_read_section(PortFD, ra_response, '#', 2, &nbytes_read);
  if(nbytes_read == 0) {
    IDLog("No response from mount when reading ER\n");
    return false;
  }
  ra_response[nbytes_read +1] = 0;
  error_type = tty_read_section(PortFD, dec_response, '#', 2, &nbytes_read);
  if(nbytes_read == 0) {
    IDLog("No response from mount when reading ED\n");
    return false;
  }
  dec_response[nbytes_read +1] = 0;
  error_type = tty_read_section(PortFD, ra_tar_response, '#', 2, &nbytes_read);
  if(nbytes_read == 0) {
    IDLog("No response from mount when reading Er\n");
    return false;
  }
  ra_tar_response[nbytes_read +1] = 0;
  error_type = tty_read_section(PortFD, dec_tar_response, '#', 2, &nbytes_read);
  if(nbytes_read == 0) {
    IDLog("No response from mount when reading Ed\n");
    return false;
  }
  dec_tar_response[nbytes_read +1] = 0;
  
  RA_gearpos = atoi(ra_response);
  DEC_gearpos  = atoi(dec_response);
  RA_gearpos_target = atoi(ra_tar_response);
  DEC_gearpos_target  = atoi(dec_tar_response);
  
  double newRA = ( (double)RA_gearpos / (double)RA_gearpos_max ) * 24.0;
  double newDEC = AxisDegreesToDec(( (double)DEC_gearpos / (double)DEC_gearpos_max ) * 360.0);
  
  NewRaDec(newRA, newDEC);
  
  IDLog("Read new position %d, %d, %d, %d as RA%f, DEC%f\n", RA_gearpos, DEC_gearpos, RA_gearpos_target, DEC_gearpos_target, newRA, newDEC);
  return true;
}

void ArduinoGEMDriver::TimerHit() {
  ReadScopeStatus();
}

char ArduinoGEMDriver::ACK() {
  char ack[6] = ":ACK#";
  char response[2];
  int nbytes_write=0, nbytes_read=0, error_type;

  nbytes_write = write(PortFD, ack, 5);

  if (nbytes_write < 0) {
    IDLog("Could not write to serial port\n");
    return -1;
  }
 
  error_type = tty_read(PortFD, response, 2, 5, &nbytes_read);
  
  if (nbytes_read == 2) {
    if(response[0] == 'G') {
      return 0;
    } else if(response[0] == '#') {
      IDLog("Out of sync with serial comms\n");
      return -1;
    } else {
      IDLog("Out of sync with serial comms\n");
      return -1;
    }
  } else {
    IDLog("No response from controller\n");
    return -1;
  }
}

bool ArduinoGEMDriver::get_static_properties() {
  char ack[9] = ":EX#:EY#";
  char ra_response[15];
  char dec_response[15];
  int nbytes_write=0, nbytes_read=0, error_type;

  nbytes_write = write(PortFD, ack, 8);

  if (nbytes_write < 0) {
    IDLog("Could not write to serial port\n");
    return false;
  }
 
  error_type = tty_read_section(PortFD, ra_response, '#', 2, &nbytes_read);
  if(nbytes_read == 0) {
    IDLog("No response from mount when reading static properties (EX)\n");
    return false;
  }
  error_type = tty_read_section(PortFD, dec_response, '#', 2, &nbytes_read);
  if(nbytes_read == 0) {
    IDLog("No response from mount when reading static properties (EY)\n");
    return false;
  }
  
  RA_gearpos_max = atoi(ra_response);
  IDLog("RA_gearpos_max=%d\n", RA_gearpos_max);
  DEC_gearpos_max  = atoi(dec_response);
  IDLog("DEC_gearpos_max=%d\n", DEC_gearpos_max);
}

void ArduinoGEMDriver::flushSerialBuffer(int timeout) {
    char flushbuffer[400];
    int nbytes_read;
    int flushresponse = tty_read(PortFD, flushbuffer, 400, timeout, &nbytes_read);
}


/* Scale for converting axis angle to dec
Quater 1,   From 0-90 degress dec = 0-90 degrees
Quater 2,   From 90-180 degress dec = 90-0 degrees
Quater 3,   From 180-270 degress dec = 0 - -90 degrees
Quater 4,   From  270-360 degress dec = -90 - 0 degrees
*/

double ArduinoGEMDriver::DECToAxisDegrees(double dec)  { // take a dec value of -90 to 90 and convert to a 360 degree angle for the physical mount axis
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










