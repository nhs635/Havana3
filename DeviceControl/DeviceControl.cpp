
#include "DeviceControl.h"

#include <QDebug>
#include <QMessageBox>

#include <Havana3/Configuration.h>

#include <DeviceControl/FaulhaberMotor/RotaryMotor.h>
#include <DeviceControl/FaulhaberMotor/PullbackMotor.h>
#include <DeviceControl/PmtGainControl/PmtGainControl.h>
#include <DeviceControl/ElforlightLaser/ElforlightLaser.h>
#include <DeviceControl/FreqDivider/FreqDivider.h>
#include <DeviceControl/AxsunControl/AxsunControl.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/SignatecDAQ/SignatecDAQ.h>
#include <DataAcquisition/AxsunCapture/AxsunCapture.h>

#include <Common/Array.h>

#include <iostream>
#include <thread>
#include <chrono>


DeviceControl::DeviceControl(Configuration *pConfig) :
    m_pConfig(pConfig),
	m_pPullbackMotor(nullptr), m_pRotaryMotor(nullptr),
	m_pPmtGainControl(nullptr), m_pElforlightLaser(nullptr),
	m_pFlimFreqDivider(nullptr), m_pAxsunFreqDivider(nullptr), 
	m_pAxsunControl(nullptr)
{
	// Message & error handling function object
	SendStatusMessage += [&](const char* msg, bool is_error) {
		QString qmsg = QString::fromUtf8(msg);
		if (is_error)
		{
			m_pConfig->writeToLog(QString("[ERROR] %1").arg(qmsg));
			QMessageBox MsgBox(QMessageBox::Critical, "Device Error", qmsg);
			MsgBox.exec();
		}
		else
		{
			m_pConfig->writeToLog(qmsg);
			qDebug() << qmsg;
		}
	};
}

DeviceControl::~DeviceControl()
{
	//setAllDeviceOff();
}


void DeviceControl::setAllDeviceOff()
{
	connectAxsunControl(false);
	startSynchronization(false);
	connectFlimLaser(false);
	applyPmtGainVoltage(false);
	connectPullbackMotor(false);
	connectRotaryMotor(false);

	SendStatusMessage("All devices are disconnected from the system.", false);
}


bool DeviceControl::connectRotaryMotor(bool toggled)
{
	if (toggled)
	{
		// Create Faulhaber rotary motor control objects
		if (!m_pRotaryMotor)
		{
			m_pRotaryMotor = new RotaryMotor;
			m_pRotaryMotor->SendStatusMessage += SendStatusMessage;
		}

		// Connect the motor
		if (!(m_pRotaryMotor->ConnectDevice()))
		{
			m_pRotaryMotor->SendStatusMessage("Faulhaber rotary motor is not connected!\nCatheter rotation failed. Cannot start live mode.", true);

			delete m_pRotaryMotor;
			m_pRotaryMotor = nullptr;

			return false;
		}
	}
	else
	{
		if (m_pRotaryMotor)
		{
			// Disconnect the motor
			m_pRotaryMotor->DisconnectDevice();

			// Delete Faulhaber motor control objects
			delete m_pRotaryMotor;
			m_pRotaryMotor = nullptr;
		}
	}

	return true;
}

void DeviceControl::rotate(bool toggled)
{
	if (m_pRotaryMotor)
	{
		if (toggled)
			m_pRotaryMotor->RotateMotor(m_pConfig->rotaryRpm);
		else
			m_pRotaryMotor->StopMotor();
	}
}

void DeviceControl::changeRotaryRpm(int rpm)
{
	m_pConfig->rotaryRpm = rpm;
		
	char msg[256];
	sprintf(msg, "[FAULHABER] Rotary RPM set: %d RPM", rpm);
	SendStatusMessage(msg, false);
}


bool DeviceControl::connectPullbackMotor(bool enabled)
{
    if (enabled)
	{
		// Create Faulhaber pullback motor control objects
		if (!m_pPullbackMotor)
		{
			m_pPullbackMotor = new PullbackMotor;
			m_pPullbackMotor->SendStatusMessage += SendStatusMessage;
		}

		// Connect the motor
		if (!(m_pPullbackMotor->ConnectDevice()))
		{
			m_pPullbackMotor->SendStatusMessage("Faulhaber pullback motor is not connected!\nCatheter pullback failed. Cannot start recording mode.", true);

			delete m_pPullbackMotor;
			m_pPullbackMotor = nullptr;

			return false;
		}
		else
		{
			m_pPullbackMotor->EnableMotor();
			this->home();
		}
	}
	else
	{
		if (m_pPullbackMotor)
		{
			// Disconnect the motor
			m_pPullbackMotor->DisableMotor();
			m_pPullbackMotor->DisconnectDevice();

			// Delete Faulhaber motor control objects
			delete m_pPullbackMotor;
			m_pPullbackMotor = nullptr;
		}
	}
	
	return true;
}

void DeviceControl::moveAbsolute()
{
	if (m_pPullbackMotor)
	{
		// Pullback
		m_pPullbackMotor->RotateMotor(-int(m_pConfig->pullbackSpeed * GEAR_RATIO));

		// Pullback end condition
		float duration = m_pConfig->pullbackLength / m_pConfig->pullbackSpeed;
		std::thread stop([&, duration]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(int(1000 * duration)));
			this->stop();
		});
		stop.detach();
	}
}

void DeviceControl::setTargetSpeed(float speed)
{
    m_pConfig->pullbackSpeed = speed;

	char msg[256];
	sprintf(msg, "[FAULHABER] Pullback target speed set: %.2f mm/sec", speed);
	SendStatusMessage(msg, false);
}

void DeviceControl::changePullbackLength(float length)
{
    m_pConfig->pullbackLength = length;

	char msg[256];
	sprintf(msg, "[FAULHABER] Pullback target length set: %.2f mm", length);
	SendStatusMessage(msg, false);
}

void DeviceControl::home()
{
	if (m_pPullbackMotor)
	    m_pPullbackMotor->RotateMotor(int(20 * GEAR_RATIO));
}

void DeviceControl::stop()
{
	if (m_pPullbackMotor)
		m_pPullbackMotor->StopMotor();
}


bool DeviceControl::applyPmtGainVoltage(bool enabled)
{
#ifdef NI_ENABLE	
    if (enabled)
	{
		// Create PMT gain control objects
		if (!m_pPmtGainControl)
		{
			m_pPmtGainControl = new PmtGainControl;
			m_pPmtGainControl->SendStatusMessage += SendStatusMessage;
		}

		if (m_pPmtGainControl->voltage > 1.0)
		{
			m_pPmtGainControl->SendStatusMessage("[PMT gain] >1.0V Gain voltage cannot be assigned!", true);
			applyPmtGainVoltage(false);
            return false;
		}

		// Initializing
		if (!m_pPmtGainControl->initialize())
		{
			applyPmtGainVoltage(false);
            return false;
		}

		// Generate PMT gain voltage
		m_pPmtGainControl->start();
	}
	else
	{
		// Delete PMT gain control objects
		if (m_pPmtGainControl)
		{
			m_pPmtGainControl->stop();
			delete m_pPmtGainControl;
			m_pPmtGainControl = nullptr;
		}
	}	
#else
    (void)enabled;
#endif

    return true;
}

void DeviceControl::changePmtGainVoltage(float voltage)
{
    m_pConfig->pmtGainVoltage = voltage;

	char msg[256];
	sprintf(msg, "[PMT gain] Gain voltage set: %.2f V", voltage);
	SendStatusMessage(msg, false);
}


bool DeviceControl::connectFlimLaser(bool state)
{
	if (state)
	{
		// Create FLIM laser power control objects
		if (!m_pElforlightLaser)
		{
			m_pElforlightLaser = new ElforlightLaser;
			m_pElforlightLaser->SendStatusMessage += SendStatusMessage;
///			m_pElforlightLaser->UpdateState += [&](double* state) {
///				m_pStreamTab->getMainWnd()->updateFlimLaserState(state);
///			};
		}

		// Connect the laser
		if (!(m_pElforlightLaser->ConnectDevice()))
		{
			m_pElforlightLaser->SendStatusMessage("[ELFORLIGHT] UV Pulsed Laser is not connected!", true);

			delete m_pElforlightLaser;
			m_pElforlightLaser = nullptr;

			return false;
		}

		return true;
	}
	else
	{
		if (m_pElforlightLaser)
		{
			// Disconnect the laser
			m_pElforlightLaser->DisconnectDevice();

			// Delete FLIM laser power control objects
			delete m_pElforlightLaser;
            m_pElforlightLaser = nullptr;
		}
		
		return true;
	}
}

void DeviceControl::adjustLaserPower(int level)
{
	//static int i = 0;
//	if (i == 0)
//	{
//		QMessageBox MsgBox;
//		MsgBox.setWindowTitle("Warning");
//		MsgBox.setIcon(QMessageBox::Warning);
//		MsgBox.setText("Increasing the laser power level can be harmful to the patient.\nWould you like to adjust the laser power level?");
//		MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
//		MsgBox.setDefaultButton(QMessageBox::No);

//		int resp = MsgBox.exec();
//		switch (resp)
//		{
//		case QMessageBox::Yes:
//			i++;
//			break;
//		case QMessageBox::No:
//			return;
//		default:
//			return;
//		}
//	}

	static int flim_laser_power_level = 0;

	if (level > flim_laser_power_level)
	{
		m_pElforlightLaser->IncreasePower();
		flim_laser_power_level++;
	}
	else
	{
		m_pElforlightLaser->DecreasePower();
		flim_laser_power_level--;
	}

	char msg[256];
	sprintf(msg, "[ELFORLIGHT] Laser power adjuset: %d", flim_laser_power_level);
	SendStatusMessage(msg, false);
}

void DeviceControl::sendLaserCommand(char* command)
{
	if (m_pElforlightLaser) 
		if (m_pElforlightLaser->isLaserEnabled)
			m_pElforlightLaser->SendCommand(command);
}


bool DeviceControl::startSynchronization(bool enabled)
{
#ifdef NI_ENABLE
	if (enabled)
	{
		// Create FLIm laser sync control objects
		if (!m_pFlimFreqDivider)
		{
			m_pFlimFreqDivider = new FreqDivider;
			m_pFlimFreqDivider->SendStatusMessage += SendStatusMessage;
		}
		m_pFlimFreqDivider->sourceTerminal = m_pConfig->flimTriggerSource;
		m_pFlimFreqDivider->counterChannel = m_pConfig->flimTriggerChannel;
		m_pFlimFreqDivider->slow = 4;

		// Create Axsun OCT sync control objects
		if (!m_pAxsunFreqDivider)
		{
			m_pAxsunFreqDivider = new FreqDivider;
			m_pAxsunFreqDivider->SendStatusMessage += SendStatusMessage;
		}
		m_pAxsunFreqDivider->sourceTerminal = m_pConfig->octTriggerSource;
		m_pAxsunFreqDivider->counterChannel = m_pConfig->octTriggerChannel;
		m_pAxsunFreqDivider->slow = 1024;

		// Initializing
		if (m_pFlimFreqDivider->initialize() && m_pAxsunFreqDivider->initialize())
		{
			// Generate FLIm laser & Axsun OCT sync
			m_pFlimFreqDivider->start();
			m_pAxsunFreqDivider->start();

			char msg[256];
			sprintf_s(msg, 256, "[SYNC] Synchronization started. (source: %s)", m_pConfig->flimTriggerSource.toLocal8Bit().data());
			SendStatusMessage(msg, false);
		}
		else
		{
			startSynchronization(false);
						
			SendStatusMessage("[SYNC ERROR] Synchronization failed.", true);
			return false;
		}
	}
	else
	{
		// Delete FLIm laser sync control objects
		if (m_pFlimFreqDivider)
		{
			m_pFlimFreqDivider->stop();
			delete m_pFlimFreqDivider;
			m_pFlimFreqDivider = nullptr;
		}

		// Delete Axsun OCT sync control objects
		if (m_pAxsunFreqDivider)
		{
			m_pAxsunFreqDivider->stop();
			delete m_pAxsunFreqDivider;
			m_pAxsunFreqDivider = nullptr;
		}
	}
#else
	(void)enabled;
#endif

	return true;
}


bool DeviceControl::connectAxsunControl(bool toggled)
{
	if (toggled)
	{
		// Create Axsun OCT control objects
		if (!m_pAxsunControl)
		{
			m_pAxsunControl = new AxsunControl;
			m_pAxsunControl->SendStatusMessage += SendStatusMessage;
///			m_pAxsunControl->DidTransferArray += [&](int i) {
///				emit transferAxsunArray(i);
///			};
			
			// Initialize the Axsun OCT control
			if (!(m_pAxsunControl->initialize()))
			{
				connectAxsunControl(false);
                return false;
			}
			
			// Default Bypass Mode
			m_pAxsunControl->setBypassMode(bypass_mode::jpeg_compressed);
		
			// Default Clock Delay
			setClockDelay(CLOCK_GAIN * CLOCK_DELAY + CLOCK_OFFSET);

			// Default VDL Length
			m_pAxsunControl->setVDLHome();
			std::thread vdl_length([&]() {
				//std::this_thread::sleep_for(std::chrono::milliseconds(1500));
				//m_pAxsunControl->setVDLLength(m_pConfig->axsunVDLLength);

				//std::this_thread::sleep_for(std::chrono::milliseconds(2000));
//				if (m_pStreamTab->getOperationTab()->getAcquisitionButton()->isChecked())
//					setVDLWidgets(true);
			});
			vdl_length.detach();

			// Default Contrast Range
			adjustDecibelRange(m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max);
			//		m_pStreamTab->getVisTab()->setOctDecibelContrastWidgets(true);
		}						
	}
	else
	{
		// Set status
		setLiveImaging(false);
		setLightSource(false);
				
		if (m_pAxsunControl)
		{
			// Delete Axsun OCT control objects
			delete m_pAxsunControl;
			m_pAxsunControl = nullptr;
		}
	}

    return true;
}

void DeviceControl::setLightSource(bool toggled)
{
	if (m_pAxsunControl)
	{
		// Start or stop Axsun light source operation
		m_pAxsunControl->setLaserEmission(toggled);		
	}
}

void DeviceControl::setLiveImaging(bool toggled)
{
	if (m_pAxsunControl)
	{
		// Start or stop Axsun live imaging operation
		m_pAxsunControl->setLiveImagingMode(toggled);	
	}
}

void DeviceControl::adjustDecibelRange(double min, double max)
{
	if (m_pAxsunControl)
	{
		m_pConfig->axsunDbRange.min = min;
		m_pConfig->axsunDbRange.max = max;
		m_pAxsunControl->setDecibelRange(m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max);
	}
}

void DeviceControl::setClockDelay(double)
{
	if (m_pAxsunControl)
		m_pAxsunControl->setClockDelay(CLOCK_DELAY);
}

void DeviceControl::setVDLLength(double length)
{
	if (m_pAxsunControl)
	{
		m_pConfig->axsunVDLLength = length;
		m_pAxsunControl->setVDLLength(length);
	}
}

void DeviceControl::setVDLHome()
{
	if (m_pAxsunControl)
	{
		m_pConfig->axsunVDLLength = 0.0f;
		m_pAxsunControl->setVDLHome();
	}
}

void DeviceControl::requestOctStatus()
{
	if (m_pAxsunControl) 
		m_pAxsunControl->getDeviceState();
}
