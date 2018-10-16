#ifndef ELFORLIGHT_LASER_H
#define ELFORLIGHT_LASER_H

#include <Common/callback.h>

#include "../QSerialComm.h"


class ElforlightLaser
{
public:
	ElforlightLaser();
	~ElforlightLaser();

public:
	bool ConnectDevice();
	void DisconnectDevice();
	void IncreasePower();
	void DecreasePower();

public:
	callback2<const char*, bool> SendStatusMessage;

private:
	QSerialComm* m_pSerialComm;
	const char* port_name;
};

#endif
