#ifndef PULLBACK_MOTOR_H
#define PULLBACK_MOTOR_H

#include <Common/callback.h>

#include "../QSerialComm.h"

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

#define GEAR_RATIO 339.6715627996165 //334.224


class PullbackMotor
{
public:
	explicit PullbackMotor();
	virtual ~PullbackMotor();

public:
	bool ConnectDevice();
	void DisconnectDevice();

	void Controlword(char value);

	void EnableMotor();
	void DisableMotor();
	void RotateMotor(int RPM);
	void StopMotor();
	//void ReadPosition();

private:
	void monitor();

public:
	inline void setPortName(const char* _port_name, int _dev_num = 1) { port_name = _port_name; dev_num = _dev_num; }
	inline int getCurrentRPM() { return current_rpm; }
	//inline void setCurrentPos(float _pos) { current_pos = _pos; }
	//inline float getCurrentPos() { return current_pos; }
	inline void setDuration(float _duration) { duration = _duration; }
	inline bool getEnableState() { return enable_state; }
	inline bool setMovingState(bool _state) { moving_state = _state; }
	inline bool getMovingState() { return moving_state; }

private:
	uint8_t CalcCRCByte(uint8_t u8Byte, uint8_t u8CRC);

private:
	QSerialComm* m_pSerialComm;
	const char* port_name;
	int dev_num;

	int current_rpm;
	float current_pos;
	float duration;
	bool enable_state;
	bool moving_state;

	char controlword[11] = { (char)0x53, (char)0x09, (char)0x01, (char)0x02, (char)0x40, (char)0x60, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x45 };
	char target_velocity[13] = { (char)0x53, (char)0x0B, (char)0x01, (char)0x02, (char)0xFF, (char)0x60, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x45 };
	//char get_current_pos[9] = { (char)0x53, (char)0x07, (char)0x02, (char)0x01, (char)0x64, (char)0x60, (char)0x00, (char)0x00, (char)0x45 };

	// thread
	std::thread _thread;

public:
	bool _running;

public:
	callback<int> DidRotateEnd;
	callback2<const char*, bool> SendStatusMessage;
	std::mutex mtx_motor;
	std::condition_variable cv_motor;
};

#endif
