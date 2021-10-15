
#include "FaulhaberMotor.h"


FaulhaberMotor::FaulhaberMotor() :
	port_name(""), dev_num(1), enable_state(false), current_rpm(0), is_moving(false), current_pos(0.0f)
{
	m_pSerialComm = new QSerialComm;
}

FaulhaberMotor::~FaulhaberMotor()
{	
	DisconnectDevice();
	if (m_pSerialComm) delete m_pSerialComm;
}


bool FaulhaberMotor::ConnectDevice()
{
	// Open a port
	if (!m_pSerialComm->m_bIsConnected)
	{
		if (m_pSerialComm->openSerialPort(port_name, QSerialPort::Baud115200))
		{
			char msg[256];
			sprintf(msg, "[FAULHABER] Success to connect to %s.", port_name);
			SendStatusMessage(msg, false);

			//m_pSerialComm->DidReadBuffer += [&](char* buffer, qint64 len)
			//{
			//	///static char msg[256];
			//	///static int j = 0;

			//	///for (int i = 0; i < (int)len; i++)
			//	///	msg[j++] = buffer[i];

			//	///if (buffer[len - 1] == '\n')
			//	///{
			//	///	msg[j] = '\0';
			//	///	//SendStatusMessage(msg, false);
			//	///	j = 0;
			//	///}

			//	///printf("FAULHABER: Receive: ");
			//	///for (int i = 0; i < (int)len; i++)
			//	///	printf("%02X ", (unsigned char)buffer[i]);
			//	///printf("\n");

			//	///DidRotateEnd();
			//};
		}
		else
		{
			char msg[256];
			sprintf(msg, "[FAULHABER] Fail to connect to %s.", port_name);
			SendStatusMessage(msg, false);
			return false;
		}
	}
	else
		SendStatusMessage("[FAULHABER] Already connected.", false);

	return true;
}

void FaulhaberMotor::DisconnectDevice()
{
	if (m_pSerialComm->m_bIsConnected)
	{
		StopMotor();

		m_pSerialComm->closeSerialPort();

		char msg[256];
		sprintf(msg, "[FAULHABER] Success to disconnect from %s.", port_name);
		SendStatusMessage(msg, false);
	}
}


void FaulhaberMotor::Controlword(char value)
{
	controlword[2] = dev_num;

	controlword[7] = (char)value;
	uint8_t crc_temp = 0xFF;
	for (int i = 1; i < controlword[1]; i++)
		crc_temp = CalcCRCByte(controlword[i], crc_temp);
	controlword[9] = (char)crc_temp;

	char msg[256];
	sprintf(msg, "[FAULHABER] Send: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", 
		(unsigned char)controlword[0], (unsigned char)controlword[1],
		(unsigned char)controlword[2], (unsigned char)controlword[3], 
		(unsigned char)controlword[4], (unsigned char)controlword[5], 
		(unsigned char)controlword[6], (unsigned char)controlword[7], 
		(unsigned char)controlword[8], (unsigned char)controlword[9], 
		(unsigned char)controlword[10]);
	SendStatusMessage(msg, false);

	m_pSerialComm->writeSerialPort(controlword, 11);
	m_pSerialComm->waitUntilResponse();
}

void FaulhaberMotor::EnableMotor()
{
	Controlword(0x06);
	Controlword(0x0F);

	char msg[256];
	sprintf(msg, "[FAULHABER] Motor enabled. [%s]", port_name);
	SendStatusMessage(msg, false);

	enable_state = true;
}

void FaulhaberMotor::DisableMotor()
{
	Controlword(0x0D);

	char msg[256];
	sprintf(msg, "[FAULHABER] Motor disabled. [%s]", port_name);
	SendStatusMessage(msg, false);

	enable_state = false;
	current_rpm = 0;
	is_moving = false;
}

void FaulhaberMotor::RotateMotor(int RPM)
{
	if (!enable_state) EnableMotor();

	target_velocity[2] = dev_num;

	RPM = -RPM; // (!FAULHABER_POSITIVE_ROTATION) ? -RPM : RPM;
	target_velocity[7] = (char)(0xFF & RPM);
	target_velocity[8] = (char)(0xFF & (RPM >> 8));
	target_velocity[9] = (char)(0xFF & (RPM >> 16));
	target_velocity[10] = (char)(0xFF & (RPM >> 24));
	uint8_t crc_temp = 0xFF;
	for (int i = 1; i < target_velocity[1]; i++)
		crc_temp = CalcCRCByte(target_velocity[i], crc_temp);
	target_velocity[11] = (char)crc_temp;

	char msg[256]; // str cat으로 교체해야되는데.
	sprintf(msg, "[FAULHABER] Send: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
		(unsigned char)target_velocity[0], (unsigned char)target_velocity[1],
		(unsigned char)target_velocity[2], (unsigned char)target_velocity[3],
		(unsigned char)target_velocity[4], (unsigned char)target_velocity[5],
		(unsigned char)target_velocity[6], (unsigned char)target_velocity[7],
		(unsigned char)target_velocity[8], (unsigned char)target_velocity[9],
		(unsigned char)target_velocity[10], (unsigned char)target_velocity[11],
		(unsigned char)target_velocity[12]);
	SendStatusMessage(msg, false);

	if (m_pSerialComm)
	{
		m_pSerialComm->writeSerialPort(target_velocity, 13);
		m_pSerialComm->waitUntilResponse();
	}

	is_moving = true;
	current_rpm = RPM;
	sprintf(msg, "[FAULHABER] Motor rotated. (%d rpm) [%s]", RPM, port_name);
	SendStatusMessage(msg, false);
}

void FaulhaberMotor::StopMotor()
{
	target_velocity[2] = dev_num;

	target_velocity[7] = (char)0x00;
	target_velocity[8] = (char)0x00;
	target_velocity[9] = (char)0x00;
	target_velocity[10] = (char)0x00;
	uint8_t crc_temp = 0xFF;
	for (int i = 1; i < target_velocity[1]; i++)
		crc_temp = CalcCRCByte(target_velocity[i], crc_temp);
	target_velocity[11] = (char)crc_temp;

	char msg[256]; // str cat으로 교체해야되는데.
	sprintf(msg, "[FAULHABER] Send: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
		(unsigned char)target_velocity[0], (unsigned char)target_velocity[1],
		(unsigned char)target_velocity[2], (unsigned char)target_velocity[3],
		(unsigned char)target_velocity[4], (unsigned char)target_velocity[5],
		(unsigned char)target_velocity[6], (unsigned char)target_velocity[7],
		(unsigned char)target_velocity[8], (unsigned char)target_velocity[9],
		(unsigned char)target_velocity[10], (unsigned char)target_velocity[11],
		(unsigned char)target_velocity[12]);
	SendStatusMessage(msg, false);

	if (m_pSerialComm)
	{
		m_pSerialComm->writeSerialPort(target_velocity, 13);
		m_pSerialComm->waitUntilResponse();
	}

	is_moving = false;
	current_rpm = 0;
	sprintf(msg, "[FAULHABER] Motor stopped. [%s]", port_name);
	SendStatusMessage(msg, false);
}

void FaulhaberMotor::MoveAbsolute(int pos)
{
	target_position[2] = dev_num;

	target_position[7] = (char)(0xFF & pos);
	target_position[8] = (char)(0xFF & (pos >> 8));
	target_position[9] = (char)(0xFF & (pos >> 16));
	target_position[10] = (char)(0xFF & (pos >> 24));
	uint8_t crc_temp = 0xFF;
	for (int i = 1; i < target_position[1]; i++)
		crc_temp = CalcCRCByte(target_position[i], crc_temp);
	target_position[11] = (char)crc_temp;

	char msg[256]; // str cat으로 교체해야되는데.
	sprintf(msg, "[FAULHABER] Send: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
		(unsigned char)target_position[0], (unsigned char)target_position[1],
		(unsigned char)target_position[2], (unsigned char)target_position[3],
		(unsigned char)target_position[4], (unsigned char)target_position[5],
		(unsigned char)target_position[6], (unsigned char)target_position[7],
		(unsigned char)target_position[8], (unsigned char)target_position[9],
		(unsigned char)target_position[10], (unsigned char)target_position[11],
		(unsigned char)target_position[12]);
	SendStatusMessage(msg, false);

	if (m_pSerialComm)
	{
		m_pSerialComm->writeSerialPort(target_position, 13);
		m_pSerialComm->waitUntilResponse();
	}

	Controlword(0x0F);
	Controlword(0x7F);

	sprintf(msg, "[FAULHABER] Motor moved absolutely. [%s]", port_name);
	SendStatusMessage(msg, false);
}

void FaulhaberMotor::Home()
{
	target_position[2] = dev_num;

	target_position[7] = (char)0x00;
	target_position[8] = (char)0x00;
	target_position[9] = (char)0x00;
	target_position[10] = (char)0x00;
	uint8_t crc_temp = 0xFF;
	for (int i = 1; i < target_position[1]; i++)
		crc_temp = CalcCRCByte(target_position[i], crc_temp);
	target_position[11] = (char)crc_temp;

	char msg[256]; // str cat으로 교체해야되는데.
	sprintf(msg, "[FAULHABER] Send: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
		(unsigned char)target_position[0], (unsigned char)target_position[1],
		(unsigned char)target_position[2], (unsigned char)target_position[3],
		(unsigned char)target_position[4], (unsigned char)target_position[5],
		(unsigned char)target_position[6], (unsigned char)target_position[7],
		(unsigned char)target_position[8], (unsigned char)target_position[9],
		(unsigned char)target_position[10], (unsigned char)target_position[11],
		(unsigned char)target_position[12]);
	SendStatusMessage(msg, false);

	if (m_pSerialComm)
	{
		m_pSerialComm->writeSerialPort(target_position, 13);
		m_pSerialComm->waitUntilResponse();
	}

	DisableMotor();

	sprintf(msg, "[FAULHABER] Motor moved to home. [%s]", port_name);
	SendStatusMessage(msg, false);
}


uint8_t FaulhaberMotor::CalcCRCByte(uint8_t u8Byte, uint8_t u8CRC)
{
	u8CRC = u8CRC ^ u8Byte;
	for (uint8_t i = 0; i < 8; i++)
	{
		if (u8CRC & 0x01)
			u8CRC = (u8CRC >> 1) ^ 0xD5;
		else
			u8CRC >>= 1;
	}

	return u8CRC;
}