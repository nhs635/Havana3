#ifndef ELFORLIGHT_LASER_H
#define ELFORLIGHT_LASER_H

#include <Common/callback.h>

#include "../QSerialComm.h"


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

	void SendCommand(char* command); 
	void MonitoringState(char* msg);

public:
	inline void setPortName(const char* _port_name) { port_name = _port_name; }
	inline bool isLaserEnabled() { return is_laser_enabled; }

private:
	QSerialComm* m_pSerialComm;
	const char* port_name;
	bool is_laser_enabled;

public:	
	callback<double*> UpdateState;
	callback2<const char*, bool> SendStatusMessage;
};

#endif
