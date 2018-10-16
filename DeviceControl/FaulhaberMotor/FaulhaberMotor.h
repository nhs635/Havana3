#ifndef FAULHABER_MOTOR_H
#define FAULHABER_MOTOR_H

#include <Common/callback.h>

#include "../QSerialComm.h"


class FaulhaberMotor 
{
public:
	FaulhaberMotor();
	~FaulhaberMotor();

public:
	bool ConnectDevice();
	void DisconnectDevice();
	void RotateMotor(int RPM);
	void StopMotor();

public:
	callback2<const char*, bool> SendStatusMessage;

private:
	QSerialComm* m_pSerialComm;
	const char* port_name;
};

#endif
