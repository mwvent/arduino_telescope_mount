#ifndef SIMPLESCOPE_H
#define SIMPLESCOPE_H

#include "inditelescope.h"

class ArduinoGEMDriver : public INDI::Telescope
{
public:
    ArduinoGEMDriver();

protected:
    // General device functions
    bool Connect();
    bool Connect(const char *port);
    bool Disconnect();
    const char *getDefaultName();
    bool initProperties();

    // Telescope specific functions
    char ACK();
    void flushSerialBuffer(int timeout);
    bool ReadScopeStatus();
    bool get_static_properties();
    bool Goto(double,double);
    bool Sync(double ra, double dec);
    bool Abort();
    bool GuideNorth(float ms);
    bool GuideSouth(float ms);
    bool GuideEast(float ms);
    bool GuideWest(float ms);
    bool MoveNS(TelescopeMotionNS dir, TelescopeMotionCommand command);
    bool MoveWE(TelescopeMotionWE dir, TelescopeMotionCommand command);
    void TimerHit();
    
private:
    double currentRA;
    double currentDEC;
    double targetRA;
    double targetDEC;
    
    double DECToAxisDegrees(double dec);
    double AxisDegreesToDec(double degrees);
    
    int RA_gearpos;
    int DEC_gearpos;
    int RA_gearpos_target;
    int DEC_gearpos_target;
    int RA_gearpos_max;
    int DEC_gearpos_max;

    unsigned int DBG_SCOPE;
};

#endif // SIMPLESCOPE_H
