
#include "IPGPhotonicsLaser.h"

#include "DigitalInput/DigitalInput.h"
#include "DigitalOutput/DigitalOutput.h"

#include <iostream>
#include <thread>
#include <chrono>

#define BOOL(x) x ? 'O' : 'X'

#define POWER_LEVEL_LINE "Dev1/port0/line0:7"
#define POWER_LATCH_LINE "Dev1/port0/line8"
#define EM_ENABLE_LINE   "Dev1/port0/line9:11"
#define MONITORING_LINE  "Dev1/port0/line12:14"


IPGPhotonicsLaser::IPGPhotonicsLaser() :
    m_pDigitalInput(nullptr), 
	m_pDigitalOutput(nullptr),
    m_dwCurDigValue(0x0000),
	port_name("")
{
	m_pSerialComm = new QSerialComm;
}

IPGPhotonicsLaser::~IPGPhotonicsLaser()
{    
	DisconnectDevice();
	if (m_pSerialComm) delete m_pSerialComm;
}


bool IPGPhotonicsLaser::ConnectDevice()
{
	// Open a port
	if (!m_pSerialComm->m_bIsConnected)
	{
        if (m_pSerialComm->openSerialPort(port_name, QSerialPort::Baud57600))
		{
			char msg[256];
            sprintf(msg, "[IPGPhotonics] Success to connect to %s.", port_name);
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

                if (msg[j - 1] == '\x0d')
                {
                    // Send received message
                    msg[j - 1] = '\n'; msg[j] = '\0';
                    char send_msg[256];
                    sprintf(send_msg, "[IPGPhotonics] Receive: %s", msg);
                    SendStatusMessage(send_msg, false);

                    // Interpret the received message
                    QStringList qmsg_list = QString::fromUtf8(msg).split(":");
                    QString input_keyword = qmsg_list[0].toUtf8().data();
                    if (qmsg_list.length() > 1)
                    {
                        double status_value = qmsg_list[1].toDouble();

                        if (input_keyword == QString("STA"))
                            GetStatus((int)status_value);
                    }

                    // Re-initialize
					j = 0;
                }
            };
		}
		else
		{
			char msg[256];
            sprintf(msg, "[IPGPhotonics] Fail to connect to %s.", port_name);
            SendStatusMessage(msg, true);
			return false;
		}
	}
	else
        SendStatusMessage("[IPGPhotonics] Already connected.", true);

	return true;
}


void IPGPhotonicsLaser::DisconnectDevice()
{
	if (m_pSerialComm->m_bIsConnected)
    {
        // Stop operation
        EnableEmission(false);
        EnableEmissionD(false);

        // Close RS-232 communication
		m_pSerialComm->closeSerialPort();

		char msg[256];
        sprintf(msg, "[IPGPhotonics] Success to disconnect to %s.", port_name);
        SendStatusMessage(msg, false);
	}
}



void IPGPhotonicsLaser::ReadStatus()
{
    char* buff = (char*)"STA\n";
    SendMessage(buff);
}


void IPGPhotonicsLaser::EnableEmission(bool enabled)
{
    char* buff = enabled ? (char*)"EMON\n" : (char*)"EMOFF\n";
    SendMessage(buff);
}


void IPGPhotonicsLaser::ReadLowerDeckTemp()
{
    char* buff = (char*)"RLDT\n";
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadCaseTemp()
{
    char* buff = (char*)"RCT\n";
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadHeadTemp()
{
    char* buff = (char*)"RHT\n";
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadBackreflection()
{
    char* buff = (char*)"RBR\n";
    SendMessage(buff);
}


void IPGPhotonicsLaser::SetCurrentSetpoint(double set_point)
{
    char buff[256];
    sprintf(buff, "SCS %.1f\n", set_point);
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadCurrentSetpoint()
{
    char* buff = (char*)"RCS\n";
    SendMessage(buff);
}


void IPGPhotonicsLaser::SetIntTrigMode(bool is_internal)
{
    char buff[256];
    sprintf(buff, "STRM %d\n", (int)is_internal);
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadIntTrigMode()
{
    char* buff = (char*)"RTRM\n";
    SendMessage(buff);
}

void IPGPhotonicsLaser::SetTrigFreq(int freq)
{
    char buff[256];
    sprintf(buff, "STF %d\n", freq);
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadIntTrigFreq()
{
    char* buff = (char*)"RITF\n";
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadExtTrigFreq()
{
    char* buff = (char*)"RETF\n";
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadMinTrigFreq(int pulse_mode)
{
    char buff[256];
    sprintf(buff, "RNTF %d\n", pulse_mode);
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadMaxTrigFreq(int pulse_mode)
{
    char buff[256];
    sprintf(buff, "RXTF %d\n", pulse_mode);
    SendMessage(buff);
}


void IPGPhotonicsLaser::SetIntModulation(bool is_internal)
{
    char buff[256];
    sprintf(buff, "SMODM %d\n", (int)is_internal);
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadIntModulation()
{
    char* buff = (char*)"RMODM\n";
    SendMessage(buff);
}

void IPGPhotonicsLaser::SetIntPowerControl(bool is_internal)
{
    char buff[256];
    sprintf(buff, "SINTXA %d\n", (int)is_internal);
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadIntPowerControl()
{
    char* buff = (char*)"RINTXA\n";
    SendMessage(buff);
}

void IPGPhotonicsLaser::SetIntEmissionControl(bool is_internal)
{
    char buff[256];
    sprintf(buff, "SXCCTL %d\n", (int)is_internal);
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadIntEmissionControl()
{
    char* buff = (char*)"RXCCTL\n";
    SendMessage(buff);
}


void IPGPhotonicsLaser::ResetError()
{
    char* buff = (char*)"RERR\n";
    SendMessage(buff);
}

void IPGPhotonicsLaser::ReadFirmwareVersion()
{
    char* buff = (char*)"RFV\n";
    SendMessage(buff);
}



void IPGPhotonicsLaser::GetStatus(int _status_value)
{
    bool is_emission = _status_value & (1 << 0);
    bool is_ext_trig = _status_value & (1 << 3);
    bool is_warm_up = _status_value & (1 << 4);
    bool is_module_startup = _status_value & (1 << 8);
    bool is_booster_power = _status_value & (1 << 9);
    bool is_int_mod = _status_value & (1 << 12);
    bool is_int_ana_con = _status_value & (1 << 13);
    bool is_24v_ps = _status_value & (1 << 14);
    bool is_laser_overheat = _status_value & (1 << 16);
    bool is_back_reflection = _status_value & (1 << 17);
    bool is_head_overheat = _status_value & (1 << 21);

    char msg[256];
    sprintf(msg, "IPGPhotonics: Status: EM [%c] EXT_TRIG [%c] WARM_UP [%c] MODULE_STARTUP [%c] BOOSTER [%c]\n",
            BOOL(is_emission), BOOL(is_ext_trig), BOOL(is_warm_up), BOOL(is_module_startup), BOOL(is_booster_power));
    SendStatusMessage(msg, false);
    sprintf(msg, "IPGPhotonics: Status: INT_MOD [%c] INT_ANA_CON [%c] 24V_PS [%c] BACKREFL [%c] OVERHEAT [%c %c]\n",
            BOOL(is_int_mod), BOOL(is_int_ana_con), BOOL(is_24v_ps), BOOL(is_back_reflection), BOOL(is_laser_overheat), BOOL(is_head_overheat));
    SendStatusMessage(msg, false);
}

void IPGPhotonicsLaser::SendMessage(char *msg)
{
    char send_msg[256];
    sprintf(send_msg, "[IPGPhotonics] Send: %s", msg);
    SendStatusMessage(send_msg, false);

    m_pSerialComm->writeSerialPort(msg);
    m_pSerialComm->waitUntilResponse();
}


void IPGPhotonicsLaser::EnableEmissionD(bool enabled)
{
    if (enabled)
    {
        // Set the AuxOFF input to HIGH
        m_dwCurDigValue |= 0x0800;
        WriteDigitalOutput(EM_ENABLE_LINE, m_dwCurDigValue);

        // Set the EE input to HIGH
        m_dwCurDigValue |= 0x0200;
        WriteDigitalOutput(EM_ENABLE_LINE, m_dwCurDigValue);

        // Set the EM input to HIGH
        m_dwCurDigValue |= 0x0400;
        WriteDigitalOutput(EM_ENABLE_LINE, m_dwCurDigValue);

        SendStatusMessage("[IPGPhotonics] Laser emission enabled", false);
    }
    else
    {
        m_dwCurDigValue &= !0x0E00;
        WriteDigitalOutput(EM_ENABLE_LINE, 0);

        SendStatusMessage("[IPGPhotonics] Laser emission disabled", false);
    }
}

void IPGPhotonicsLaser::SetPowerLevel(uint32_t power_level)
{
    m_dwCurDigValue |= (power_level & 0x00FF);
    WriteDigitalOutput(POWER_LEVEL_LINE, m_dwCurDigValue);

    m_dwCurDigValue |= 0x0100;
    WriteDigitalOutput(POWER_LATCH_LINE, m_dwCurDigValue);

    m_dwCurDigValue &= !0x0100;
    WriteDigitalOutput(POWER_LATCH_LINE, m_dwCurDigValue);


    char msg[256];
    sprintf(msg, "[IPGPhotonics] Laser power adjusted: %d", power_level);
    SendStatusMessage(msg, false);
}

void IPGPhotonicsLaser::MonitorLaserStatus()
{
    uint32_t monitor_value = 0;
    ReadDigitalInput(MONITORING_LINE, monitor_value);
    m_dwCurDigValue |= monitor_value;

    monitor_value = (m_dwCurDigValue & 0x0700) >> 8;
    switch (monitor_value)
    {
        case 0:
            SendStatusMessage("[IPGPhotonics] Laser temperature is out of the operating temperature range.", false);
            break;
        case 1:
            SendStatusMessage("[IPGPhotonics] External +24 VDC supply voltage is out of the specified range.", false);
            break;
        case 4:
            SendStatusMessage("[IPGPhotonics] Normal operation.", false);
            break;
        case 5:
            SendStatusMessage("[IPGPhotonics] Laser is not ready for emission.", false);
            break;
        case 2:
            SendStatusMessage("[IPGPhotonics] Laser automatically switches OFF due to high optical power reflected back to the laser.", false);
            break;
        case 3:
            SendStatusMessage("[IPGPhotonics] Laser protection system detects internal failure.", false);
            break;
    }
}


void IPGPhotonicsLaser::ReadDigitalInput(const char* line_name, uint32_t& value)
{
    // Create NI digital input objects
    if (!m_pDigitalInput)
    {
        m_pDigitalInput = new DigitalInput;
		m_pDigitalInput->setLineName(line_name);
        m_pDigitalInput->SendStatusMessage += [&](const char* msg, bool is_error) {
            // SendStatusMessage(msg, is_error);
        };
    }

    // Initializing
    if (m_pDigitalInput->initialize())
    {
        // Read digital voltage
        value = m_pDigitalInput->start();
        m_pDigitalInput->stop();
    }

    // Delete NI digital input objects
    if (m_pDigitalInput)
    {
        delete m_pDigitalInput;
        m_pDigitalInput = nullptr;
    }
}

void IPGPhotonicsLaser::WriteDigitalOutput(const char* line_name, uint32_t value)
{
    // Create NI digital output objects
    if (!m_pDigitalOutput)
    {
		m_pDigitalOutput = new DigitalOutput;
		m_pDigitalOutput->setLineName(line_name);
        m_pDigitalOutput->SendStatusMessage += [&](const char* msg, bool is_error) {
            // SendStatusMessage(msg, is_error);
        };
    }
	m_pDigitalOutput->setValue(value);

    // Initializing
    if (m_pDigitalOutput->initialize())
    {
        // Generate digital voltage
        m_pDigitalOutput->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        m_pDigitalOutput->stop();
    }

    // Delete NI digital output objects
    if (m_pDigitalOutput)
    {
        delete m_pDigitalOutput;
        m_pDigitalOutput = nullptr;
    }
}
