
#include "ArduinoMCU.h"
#include <Havana3/Configuration.h>


#ifndef NI_ENABLE

ArduinoMCU::ArduinoMCU() :
	port_name(ARDUINO_PORT)
{
	m_pSerialComm = new QSerialComm;
}


ArduinoMCU::~ArduinoMCU()
{
	DisconnectDevice();
	if (m_pSerialComm) delete m_pSerialComm;
}


bool ArduinoMCU::ConnectDevice()
{
	// Open a port
	if (!m_pSerialComm->m_bIsConnected)
	{
		if (m_pSerialComm->openSerialPort(port_name))
		{
			char msg[256];
			sprintf(msg, "ARDUINO: Success to connect to %s.", port_name);
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

            char* buff = (char*)"Hello Arduino!\n";
			sprintf(msg, "ARDUINO: %s", buff);
			SendStatusMessage(msg, false);

            m_pSerialComm->writeSerialPort(buff);
            m_pSerialComm->waitUntilResponse();
		}
		else
		{
			char msg[256];
			sprintf(msg, "ARDUINO: Fail to connect to %s.", port_name);
			SendStatusMessage(msg, false);
			return false;
		}
	}
	else
		SendStatusMessage("ARDUINO: Already connected.", false);

	return true;
}


void ArduinoMCU::DisconnectDevice()
{
	if (m_pSerialComm->m_bIsConnected)
    {
        AdjustGainLevel(0);
        m_pSerialComm->waitUntilResponse();

        SyncDevice(false);
        m_pSerialComm->waitUntilResponse();

		m_pSerialComm->closeSerialPort();

		char msg[256];
		sprintf(msg, "ARDUINO: Success to disconnect to %s.", port_name);
		SendStatusMessage(msg, false);
	}
}


void ArduinoMCU::SyncDevice(bool state)
{
    char* buff = state ? (char*)"sync_on\n" : (char*)"sync_off\n";

	char msg[256];
	sprintf(msg, "ARDUINO: Send: %s", buff);
	SendStatusMessage(msg, false);

    m_pSerialComm->writeSerialPort(buff);
    m_pSerialComm->waitUntilResponse();
}


void ArduinoMCU::AdjustGainLevel(int gain_level)
{
    char buff[100];
    if ((gain_level > 4095) || (gain_level < 0))
        gain_level = 0;

    sprintf_s(buff, sizeof(buff), "g-%d\n", gain_level);
    
	char msg[256];
	sprintf(msg, "ARDUINO: Send: %s", buff);
	SendStatusMessage(msg, false);

    m_pSerialComm->writeSerialPort(buff);
    m_pSerialComm->waitUntilResponse();
}


void ArduinoMCU::CheckGainVoltage()
{
    char* buff = (char*)"check_gain\n";
    
	char msg[256];
	sprintf(msg, "ARDUINO: Send: %s", buff);
	SendStatusMessage(msg, false);

    m_pSerialComm->writeSerialPort(buff);
    m_pSerialComm->waitUntilResponse();
}

#endif