
#include "FaulhaberMotor.h"
#include <Havana3/Configuration.h>


FaulhaberMotor::FaulhaberMotor() :
	port_name(FAULHABER_PORT)
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
		if (m_pSerialComm->openSerialPort(port_name))
		{
			char msg[256];
			sprintf(msg, "FAULHABER: Success to connect to %s.", port_name);
			SendStatusMessage(msg, false);

			m_pSerialComm->DidReadBuffer += [&](char* buffer, qint64 len)
			{
				static char msg[256];
				static int j = 0;

				for (int i = 0; i < (int)len; i++)
					msg[j++] = buffer[i];

				if (buffer[len - 1] == '\n')
				{
					msg[j] = '\0';
					SendStatusMessage(msg, false);
					j = 0;
				}
			};
			//char* buff = (char*)"en\n";
			//sprintf(msg, "FAULHABER: Send: %s", buff);
			//SendStatusMessage(msg, false);

			//m_pSerialComm->writeSerialPort(buff);
			//m_pSerialComm->waitUntilResponse();
		}
		else
		{
			char msg[256];
			sprintf(msg, "FAULHABER: Fail to connect to %s.", port_name);
			SendStatusMessage(msg, false);
			return false;
		}
	}
	else
		SendStatusMessage("FAULHABER: Already connected.", false);

	return true;
}


void FaulhaberMotor::DisconnectDevice()
{
	if (m_pSerialComm->m_bIsConnected)
	{
		StopMotor();

		m_pSerialComm->closeSerialPort();

		char msg[256];
		sprintf(msg, "FAULHABER: Success to disconnect to %s.", port_name);
		SendStatusMessage(msg, false);
	}
}


void FaulhaberMotor::RotateMotor(int RPM)
{
	char msg[256];

	char* buff1 = (char*)"en\n";
	sprintf(msg, "FAULHABER: Send: %s", buff1);
	SendStatusMessage(msg, false);
	m_pSerialComm->writeSerialPort(buff1);
	m_pSerialComm->waitUntilResponse();

	char buff2[100];
	sprintf_s(buff2, sizeof(buff2), "v%d\n", (!FAULHABER_POSITIVE_ROTATION) ? -RPM : RPM);
	sprintf(msg, "FAULHABER: Send: %s", buff2);
	SendStatusMessage(msg, false);

	m_pSerialComm->writeSerialPort(buff2);
	m_pSerialComm->waitUntilResponse();
}


void FaulhaberMotor::StopMotor()
{
	char msg[256];

	char* buff1 = (char*)"v0\n";
	sprintf(msg, "FAULHABER: Send: %s", buff1);
	SendStatusMessage(msg, false);

	m_pSerialComm->writeSerialPort(buff1);
	m_pSerialComm->waitUntilResponse();

	char* buff2 = (char*)"di\n";
	sprintf(msg, "FAULHABER: Send: %s", buff2);
	SendStatusMessage(msg, false);
	m_pSerialComm->writeSerialPort(buff2);
	m_pSerialComm->waitUntilResponse();
}
