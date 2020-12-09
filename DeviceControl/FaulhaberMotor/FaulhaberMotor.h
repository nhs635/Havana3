#ifndef FAULHABER_MOTOR_H
#define FAULHABER_MOTOR_H

#include <Common/callback.h>

#include "../QSerialComm.h"


class FaulhaberMotor 
{
public:
	explicit FaulhaberMotor();
	virtual ~FaulhaberMotor();

public:	
	bool ConnectDevice();
	void DisconnectDevice();
	
	void Controlword(char value);

	void EnableMotor();
	void DisableMotor();
	void RotateMotor(int RPM);
	void StopMotor();
	void MoveAbsolute(int pos);
	void Home();

public:
	inline void setPortName(const char* _port_name, int _dev_num = 1) { port_name = _port_name; dev_num = _dev_num; }

private:
	uint8_t CalcCRCByte(uint8_t u8Byte, uint8_t u8CRC);
	
private:
	QSerialComm* m_pSerialComm;
	const char* port_name;
	int dev_num;

	char controlword[11] = { (char)0x53, (char)0x09, (char)0x01, (char)0x02, (char)0x40, (char)0x60, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x45 };
	char target_velocity[13] = { (char)0x53, (char)0x0B, (char)0x01, (char)0x02, (char)0xFF, (char)0x60, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x45 };
	char target_position[13] = { (char)0x53, (char)0x0B, (char)0x01, (char)0x02, (char)0x7A, (char)0x60, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x45 };
	
public:
	callback<void> DidRotateEnd;
	callback2<const char*, bool> SendStatusMessage;
};

#endif
