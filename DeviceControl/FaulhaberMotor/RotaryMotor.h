#ifndef ROTARY_MOTOR_H
#define ROTARY_MOTOR_H

#include <Common/callback.h>

#include "../QSerialComm.h"


class RotaryMotor
{
public:
	explicit RotaryMotor();
	virtual ~RotaryMotor();

public:
	bool ConnectDevice();
	void DisconnectDevice();

	void Controlword(char value);

	void EnableMotor();
	void DisableMotor();
	void RotateMotor(int RPM);
	void StopMotor();

public:
	inline void setPortName(const char* _port_name, int _dev_num = 1) { port_name = _port_name; dev_num = _dev_num; }
	inline int getCurrentRPM() { return current_rpm; }
	inline bool getEnableState() { return enable_state; }	
	inline bool getMovingState() { return moving_state; }

private:
	uint8_t CalcCRCByte(uint8_t u8Byte, uint8_t u8CRC);

private:
	QSerialComm* m_pSerialComm;
	const char* port_name;
	int dev_num;

	int current_rpm;
	bool enable_state;	
	bool moving_state;

	char controlword[11] = { (char)0x53, (char)0x09, (char)0x01, (char)0x02, (char)0x40, (char)0x60, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x45 };
	char target_velocity[13] = { (char)0x53, (char)0x0B, (char)0x01, (char)0x02, (char)0xFF, (char)0x60, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x45 };

public:
	callback<void> DidRotateEnd;
	callback2<const char*, bool> SendStatusMessage;
};

#endif
