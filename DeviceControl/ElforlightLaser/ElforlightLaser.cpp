
#include "ElforlightLaser.h"
#include <Havana3/Configuration.h>

#include <iostream>
#include <thread>
#include <chrono>


ElforlightLaser::ElforlightLaser() :
	port_name(ELFORLIGHT_PORT)
{
	m_pSerialComm = new QSerialComm;
}


ElforlightLaser::~ElforlightLaser()
{
	DisconnectDevice();
	if (m_pSerialComm) delete m_pSerialComm;
}


bool ElforlightLaser::ConnectDevice()
{
	// Open a port
	if (!m_pSerialComm->m_bIsConnected)
	{
		if (m_pSerialComm->openSerialPort(port_name))
		{
			char msg[256];
			sprintf(msg, "ELFORLIGHT: Success to connect to %s.", port_name);
			SendStatusMessage(msg, false);

			m_pSerialComm->DidReadBuffer += [&](char* buffer, qint64 len)
			{
				static char msg[256];
				static int j = 0;
				
				for (int i = 0; i < (int)len; i++)
					msg[j++] = buffer[i];

				if (buffer[len - 1] == '\x11')
				{
					msg[j] = '\0';
					SendStatusMessage(msg, false);
					j = 0;
				}
			};
		}
		else
		{
			char msg[256];
			sprintf(msg, "ELFORLIGHT: Fail to connect to %s.", port_name);
			SendStatusMessage(msg, false);
			return false;
		}
	}
	else
		SendStatusMessage("ELFORLIGHT: Already connected.", false);

	return true;
}


void ElforlightLaser::DisconnectDevice()
{
	if (m_pSerialComm->m_bIsConnected)
	{
		m_pSerialComm->closeSerialPort();

		char msg[256];
		sprintf(msg, "ELFORLIGHT: Success to disconnect to %s.", port_name);
		SendStatusMessage(msg, false);
	}
}


void ElforlightLaser::IncreasePower()
{
	char buff[2] = "+";

	char msg[256];
	sprintf(msg, "ELFORLIGHT: Send: %s", buff);
	SendStatusMessage(msg, false);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	m_pSerialComm->writeSerialPort(buff);
    m_pSerialComm->waitUntilResponse();
}


void ElforlightLaser::DecreasePower()
{
	char buff[2] = "-";

	char msg[256];
	sprintf(msg, "ELFORLIGHT: Send: %s", buff);
	SendStatusMessage(msg, false);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	m_pSerialComm->writeSerialPort(buff);
    m_pSerialComm->waitUntilResponse();
}
