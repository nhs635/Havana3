
#include "ElforlightLaser.h"

#include <iostream>
#include <thread>
#include <chrono>


ElforlightLaser::ElforlightLaser() :
	port_name(""), is_laser_enabled(false)
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
			sprintf(msg, "[ELFORLIGHT] Success to connect to %s.", port_name);
			SendStatusMessage(msg, false);

			m_pSerialComm->DidReadBuffer += [&](char* buffer, qint64 len)
			{
				static char msg[256];
				static int j = 0;
				
				for (int i = 0; i < (int)len; i++)
				{
					msg[j++] = buffer[i];
					if (j > 255) j = 0;
				}

				if ((buffer[len - 1] == '\x11') || (buffer[len - 3] == 'r'))
				{
					msg[j] = '\0';

					// j: length of msg, (60~80: '?' / 5~20: '+' or '-' or 'E' or 'R')
					if ((j > 60) && (j <= 80))
						MonitoringState(msg);
					else if (j <= 60)
						SendStatusMessage(msg, false);

					j = 0;
				}
			};

			SendCommand((char*)"E");
			SendCommand((char*)"2");
		}
		else
		{
			char msg[256];
			sprintf(msg, "[ELFORLIGHT] Fail to connect to %s.", port_name);
			SendStatusMessage(msg, false);
			return false;
		}
	}
	else
		SendStatusMessage("[ELFORLIGHT] Already connected.", false);

	return true;
}


void ElforlightLaser::DisconnectDevice()
{
	if (m_pSerialComm->m_bIsConnected)
	{
		SendCommand((char*)"R");
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		m_pSerialComm->closeSerialPort();

		char msg[256];
		sprintf(msg, "[ELFORLIGHT] Success to disconnect to %s.", port_name);
		SendStatusMessage(msg, false);
	}
}


void ElforlightLaser::IncreasePower()
{
	char buff[2] = "+";

	char msg[256];
	sprintf(msg, "[ELFORLIGHT] Send: %s", buff);
	SendStatusMessage(msg, false);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	m_pSerialComm->writeSerialPort(buff);
    m_pSerialComm->waitUntilResponse();
}


void ElforlightLaser::DecreasePower()
{
	char buff[2] = "-";

	char msg[256];
	sprintf(msg, "[ELFORLIGHT] Send: %s", buff);
	SendStatusMessage(msg, false);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	m_pSerialComm->writeSerialPort(buff);
    m_pSerialComm->waitUntilResponse();
}


void ElforlightLaser::SendCommand(char* command)
{
	char msg[256];
	if (command[0] != '?')
	{
		sprintf(msg, "[ELFORLIGHT] Send: %s", command);
		SendStatusMessage(msg, false);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	m_pSerialComm->writeSerialPort(command);
	m_pSerialComm->waitUntilResponse();
}


void ElforlightLaser::MonitoringState(char* msg)
{
	// Get message
	QString message(msg);

	// Split message
	QStringList msg_list = message.split("\n");
	if (msg_list.length() > 3)
	{
		QStringList current_msg_list = msg_list[1].split(" ");
		QStringList diode_msg_list = msg_list[2].split(" ");
		QStringList chipset_msg_list = msg_list[3].split(" ");

		// Sine fitting coefficients
		double a0 = 5.006;
		double b0 = 0.002488;
		double c0 = 7.83e-17;

		// Gaussian fitting coefficients
		double a1 = 237.0;
		double b1 = 3164.0;
		double c1 = 1778.0;

		// Monitoring value
		double is = 0.0, id = 0.0, ds = 0.0, dm = 0.0, cs = 0.0, cm = 0.0;
		if (current_msg_list.length() > 4)
		{
			is = a0 * sin(b0 * current_msg_list[2].toDouble() + c0); // Diode Current Set
			id = a0 * sin(b0 * current_msg_list[4].toDouble() + c0); // Diode Current Monitor
		}
		if (diode_msg_list.length() > 4)
		{
			ds = a1 * exp(-pow((diode_msg_list[2].toDouble() - b1) / c1, 2)); // Diode Temperature Set
			dm = a1 * exp(-pow((diode_msg_list[4].toDouble() - b1) / c1, 2)); // Diode Temperature Monitor
		}
		if (chipset_msg_list.length() > 4)
		{
			cs = a1 * exp(-pow((chipset_msg_list[2].toDouble() - b1) / c1, 2)); // Chipset Temperature Set
			cm = a1 * exp(-pow((chipset_msg_list[4].toDouble() - b1) / c1, 2)); // Chipset Temperature Monitor
		}

		// Update state
		double state[6] = { is, id, ds, dm, cs, cm };
		UpdateState(state);
	}
}