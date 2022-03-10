#ifndef ELFORLIGHT_LASER_H
#define ELFORLIGHT_LASER_H

#include <Common/callback.h>

#include "../QSerialComm.h"

#include <iostream>
#include <mutex>


class ElforlightLaser
{
public:
	explicit ElforlightLaser();
	virtual ~ElforlightLaser();

public:
	bool ConnectDevice();
	void DisconnectDevice();

	void IncreasePower();
	void DecreasePower();
	void GetCurrentPower();

	void SendCommand(char* command); 
	void MonitoringState(char* msg);

public:
	inline void setPortName(const char* _port_name) { port_name = _port_name; }
	inline bool isLaserEnabled() { return is_laser_enabled; }
	inline int getLaserPowerLevel() { return laser_power_level; }

private:
	QSerialComm* m_pSerialComm;
	const char* port_name;
	bool is_laser_enabled;
	int laser_power_level;
	std::mutex mtx_power;

public:	
	callback<double*> UpdateState;
	callback<int> UpdatePowerLevel;
	callback2<const char*, bool> SendStatusMessage;
};

#endif
