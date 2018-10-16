#ifndef ARDUINO_MCU_H
#define ARDUINO_MCU_H

#include <Common/callback.h>

#include "../QSerialComm.h"


class ArduinoMCU 
{
public:
	ArduinoMCU();
	~ArduinoMCU();

public:
	bool ConnectDevice();
	void DisconnectDevice();
    void SyncDevice(bool state);
    void AdjustGainLevel(int gain_level);
    void CheckGainVoltage();

public:
	callback2<const char*, bool> SendStatusMessage;

private:
	QSerialComm* m_pSerialComm;
	const char* port_name;
};

#endif
