#ifndef IPG_PHOTONICS_LASER_H
#define IPG_PHOTONICS_LASER_H

#include <Common/callback.h>

#include "../QSerialComm.h"


class DigitalInput;
class DigitalOutput;

class IPGPhotonicsLaser
{
//// Methods ////
public:
    explicit IPGPhotonicsLaser();
    virtual ~IPGPhotonicsLaser();
	
public:
    // Connect & Disconnect [RS232]
	bool ConnectDevice();
	void DisconnectDevice();

    // RS232 Commands
    void ReadStatus();

    void EnableEmission(bool enabled);

    void ReadLowerDeckTemp();
    void ReadCaseTemp();
    void ReadHeadTemp();
    void ReadBackreflection();

    void SetCurrentSetpoint(double set_point);
    void ReadCurrentSetpoint();

    void SetIntTrigMode(bool is_internal);
    void ReadIntTrigMode();
    void SetTrigFreq(int freq);
    void ReadIntTrigFreq();
    void ReadExtTrigFreq();
    void ReadMinTrigFreq(int pulse_mode);
    void ReadMaxTrigFreq(int pulse_mode);

    void SetIntModulation(bool is_internal);
    void ReadIntModulation();
    void SetIntPowerControl(bool is_internal);
    void ReadIntPowerControl();
    void SetIntEmissionControl(bool is_internal);
    void ReadIntEmissionControl();

    void ResetError();
    void ReadFirmwareVersion();

    void GetStatus(int _status_value);
    void SendMessage(char* msg);

public:
    // Digital Input/Output (Digital 25-pin)
    void EnableEmissionD(bool enabled);
    void SetPowerLevel(uint32_t power_level);
    void MonitorLaserStatus();

    void ReadDigitalInput(const char* line_name, uint32_t& value);
    void WriteDigitalOutput(const char* line_name, uint32_t value);
	
public:
	inline void SetPortName(const char* _port_name) { port_name = _port_name; }

//// Variables ////
private:
	QSerialComm* m_pSerialComm;

    DigitalInput* m_pDigitalInput;
    DigitalOutput* m_pDigitalOutput;

    uint32_t m_dwCurDigValue;
	const char* port_name;

public:
	callback2<const char*, bool> SendStatusMessage;
};

#endif
