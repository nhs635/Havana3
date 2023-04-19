
#include "DeviceControl.h"

#include <QDebug>
#include <QMessageBox>

#include <Havana3/Configuration.h>

#include <DeviceControl/FaulhaberMotor/RotaryMotor.h>
#include <DeviceControl/FaulhaberMotor/PullbackMotor.h>
#ifdef NI_ENABLE
#include <DeviceControl/PmtGainControl/PmtGainControl.h>
#endif
#ifndef NEXT_GEN_SYSTEM
#include <DeviceControl/ElforlightLaser/ElforlightLaser.h>
#else
#include <DeviceControl/IPGPhotonicsLaser/IPGPhotonicsLaser.h>
#endif
#ifdef NI_ENABLE
#include <DeviceControl/FreqDivider/FreqDivider.h>
#endif
#ifdef AXSUN_ENABLE
#include <DeviceControl/AxsunControl/AxsunControl.h>
#endif

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/SignatecDAQ/SignatecDAQ.h>
#ifdef AXSUN_ENABLE
#include <DataAcquisition/AxsunCapture/AxsunCapture.h>
#endif

#include <Common/Array.h>

#include <iostream>
#include <thread>
#include <chrono>


DeviceControl::DeviceControl(Configuration *pConfig) :
    m_pConfig(pConfig),
	m_pPullbackMotor(nullptr), m_pRotaryMotor(nullptr),
	m_pPmtGainControl(nullptr), m_pElforlightLaser(nullptr), m_pIPGPhotonicsLaser(nullptr),
	m_pFlimLaserFreqDivider(nullptr), m_pFlimDaqFreqDivider(nullptr), m_pAxsunFreqDivider(nullptr), 
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
}


void DeviceControl::turnOffAllDevices()
{
	// Axsun OCT operation turn off
	setLightSource(false);
#ifndef NEXT_GEN_SYSTEM
	setLiveImaging(false);
#endif

	// Stop synchronization
	startSynchronization(false);

	// PMT gain voltage turn off
	applyPmtGainVoltage(false);

	// Stop rotating
	rotateStop();
}

void DeviceControl::disconnectAllDevices()
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
			m_pRotaryMotor->setPortName(ROTARY_MOTOR_COM_PORT, 1);

			m_pRotaryMotor->SendStatusMessage += SendStatusMessage;
		}
		else
			return true;

		// Connect the motor
		if (!(m_pRotaryMotor->ConnectDevice()))
		{
			m_pRotaryMotor->SendStatusMessage("Faulhaber rotary motor is not connected!\nCatheter rotation failed. Cannot start live mode.", true);

			delete m_pRotaryMotor;
			m_pRotaryMotor = nullptr;

			return false;
		}

		//m_pRotaryMotor->EnableMotor();
	}
	else
	{
		if (m_pRotaryMotor)
		{
			// Disconnect the motor
			m_pRotaryMotor->DisableMotor();
			m_pRotaryMotor->DisconnectDevice();

			// Delete Faulhaber motor control objects
			delete m_pRotaryMotor;
			m_pRotaryMotor = nullptr;
		}
	}

	return true;
}

void DeviceControl::changeRotaryRpm(int rpm)
{
	m_pConfig->rotaryRpm = rpm;
	if (m_pRotaryMotor)
	{
		if (rpm != 0)
			m_pRotaryMotor->RotateMotor(m_pConfig->rotaryRpm);
		else
			m_pRotaryMotor->StopMotor();
	}

	char msg[256];
	sprintf(msg, "[FAULHABER] Rotary RPM set: %d RPM", rpm);
	SendStatusMessage(msg, false);
}

void DeviceControl::rotateStop()
{
	m_pConfig->rotaryRpm = 0;
	if (m_pRotaryMotor)
		m_pRotaryMotor->DisableMotor();
}


bool DeviceControl::connectPullbackMotor(bool enabled)
{
    if (enabled)
	{
		// Create Faulhaber pullback motor control objects
		if (!m_pPullbackMotor)
		{
			m_pPullbackMotor = new PullbackMotor;
			m_pPullbackMotor->setPortName(PULLBACK_MOTOR_COM_PORT, 2); 

			m_pPullbackMotor->SendStatusMessage += SendStatusMessage;
		}
		else
			return true;

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
		}
	}
	else
	{
		if (m_pPullbackMotor)
		{
			// Disconnect the motor
			m_pPullbackMotor->cv_motor.notify_one();
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
		float pullback_slowing_factor = 4.0f;
		int rpm = -int(m_pConfig->pullbackSpeed / (m_pConfig->axsunPipelineMode == 0 ? 1.0f : pullback_slowing_factor) * GEAR_RATIO);
		float duration = m_pConfig->pullbackLength / (m_pConfig->pullbackSpeed / (m_pConfig->axsunPipelineMode == 0 ? 1.0f : pullback_slowing_factor));
		m_pPullbackMotor->setDuration(duration);
		m_pPullbackMotor->RotateMotor(rpm);
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
	{
		// Pullback
		int rpm = int(m_pConfig->pullbackSpeed * GEAR_RATIO / 1.5f);
		float duration = 1.2 * 1.5f * m_pConfig->pullbackLength / m_pConfig->pullbackSpeed;
		m_pPullbackMotor->setDuration(duration);
		m_pPullbackMotor->RotateMotor(rpm);
	}
}

void DeviceControl::stop()
{
	if (m_pPullbackMotor)
	{		
		if (m_pPullbackMotor->getMovingState())
			m_pPullbackMotor->cv_motor.notify_one();
		else
			m_pPullbackMotor->StopMotor();
	}
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
			m_pPmtGainControl->setPhysicalChannel(PMT_GAIN_AO_PORT);
			m_pPmtGainControl->setVoltage(m_pConfig->pmtGainVoltage);

			m_pPmtGainControl->SendStatusMessage += SendStatusMessage;
		}
		else
			return true;

		if (m_pPmtGainControl->getVoltage() > 1.0)
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
#ifndef NEXT_GEN_SYSTEM
		// Create FLIM laser power control objects
		if (!m_pElforlightLaser)
		{
			m_pElforlightLaser = new ElforlightLaser;
			m_pElforlightLaser->setPortName(FLIM_LASER_COM_PORT);

			m_pElforlightLaser->SendStatusMessage += SendStatusMessage;
			///m_pElforlightLaser->UpdateState += [&](double* state) {
			///	//m_pStreamTab->getMainWnd()->updateFlimLaserState(state);
			///};
		}
		else
			return true;

		// Connect the laser
		if (!(m_pElforlightLaser->ConnectDevice()))
		{
			m_pElforlightLaser->SendStatusMessage("[ELFORLIGHT] UV Pulsed Laser is not connected!", true);

			delete m_pElforlightLaser;
			m_pElforlightLaser = nullptr;

			return false;
		}

		// Check current power level
		m_pElforlightLaser->GetCurrentPower();
		m_pConfig->laserPowerLevel = m_pElforlightLaser->getLaserPowerLevel();
#else
		// Create FLIM laser control objects
		if (!m_pIPGPhotonicsLaser)
		{
			m_pIPGPhotonicsLaser = new IPGPhotonicsLaser;
			m_pIPGPhotonicsLaser->SetPortName(FLIM_LASER_COM_PORT);

			m_pIPGPhotonicsLaser->SendStatusMessage += SendStatusMessage;
			m_pIPGPhotonicsLaser->InterpretReceivedMessage += [&](const char* msg) {
				// Interpret the received message
				QStringList qmsg_list = QString::fromUtf8(msg).split(":");
				QString input_keyword = qmsg_list[0].toUtf8().data();
				if (qmsg_list.length() > 1)
				{
					double status_value = qmsg_list[1].toDouble();

					if (input_keyword == QString("STA"))
						m_pIPGPhotonicsLaser->GetStatus((int)status_value);
					else if (input_keyword == QString("RCS"))
						printf("%f\n", status_value);
				}
			};
		}
		else
			return false;

		// Connect the laser
		if (!(m_pIPGPhotonicsLaser->ConnectDevice()))
		{
			m_pIPGPhotonicsLaser->SendStatusMessage("[IPGPhotonics] UV Pulsed Laser is not connected!", true);

			delete m_pIPGPhotonicsLaser;
			m_pIPGPhotonicsLaser = nullptr;

			return false;
		}
		else
		{
			// Default setup
			m_pIPGPhotonicsLaser->SetIntTrigMode(false);
			m_pIPGPhotonicsLaser->SetIntModulation(false);
			m_pIPGPhotonicsLaser->SetIntPowerControl(false);
			m_pIPGPhotonicsLaser->SetIntEmissionControl(false);
			m_pIPGPhotonicsLaser->EnableEmissionD(true);
		}
#endif
		return true;
	}
	else
	{
#ifndef NEXT_GEN_SYSTEM
		if (m_pElforlightLaser)
		{
			// Disconnect the laser
			m_pElforlightLaser->DisconnectDevice();

			// Delete FLIM laser power control objects
			delete m_pElforlightLaser;
            m_pElforlightLaser = nullptr;
		}
#else
		if (m_pIPGPhotonicsLaser)
		{
			// Disconnect the laser
			m_pIPGPhotonicsLaser->DisconnectDevice();

			// Delete FLIM laser power control objects
			delete m_pIPGPhotonicsLaser;
			m_pIPGPhotonicsLaser = nullptr;
		}
#endif		
		return true;
	}
}

void DeviceControl::adjustLaserPower(int level)
{
#ifndef NEXT_GEN_SYSTEM
	if (level > 0)
		m_pElforlightLaser->IncreasePower();
	else
		m_pElforlightLaser->DecreasePower();

	m_pConfig->laserPowerLevel = m_pElforlightLaser->getLaserPowerLevel();

	char msg[256];
	sprintf(msg, "[ELFORLIGHT] Laser power adjusted: %d", m_pConfig->laserPowerLevel);
	SendStatusMessage(msg, false);
#else
	if (m_pIPGPhotonicsLaser)
		m_pIPGPhotonicsLaser->SetPowerLevel((uint8_t)level);

	char msg[256];
	sprintf(msg, "[IPGPhotonics] Laser power adjusted: %d", (uint8_t)level);
	SendStatusMessage(msg, false);
#endif
}

void DeviceControl::sendLaserCommand(char* command)
{
#ifndef NEXT_GEN_SYSTEM
	if (m_pElforlightLaser) 
		///if (m_pElforlightLaser->isLaserEnabled())
		m_pElforlightLaser->SendCommand(command);
#else
	(void)command;
#endif
}

void DeviceControl::monitorLaserStatus()
{
#ifndef NEXT_GEN_SYSTEM

#else
	if (m_pIPGPhotonicsLaser)
	{
		m_pIPGPhotonicsLaser->ReadStatus();
		m_pIPGPhotonicsLaser->MonitorLaserStatus();
		m_pIPGPhotonicsLaser->ReadExtTrigFreq();
	}
#endif
}


bool DeviceControl::startSynchronization(bool enabled, bool async)
{
#ifdef NI_ENABLE
	if (enabled)
	{
		// Create FLIm laser sync control objects
		if (!m_pFlimLaserFreqDivider)
		{
			m_pFlimLaserFreqDivider = new FreqDivider;
			m_pFlimLaserFreqDivider->setSourceTerminal(!async ? FLIM_LASER_SOURCE_TERMINAL : "100kHzTimebase");
			m_pFlimLaserFreqDivider->setCounterChannel(FLIM_LASER_COUNTER_CHANNEL);
			m_pFlimLaserFreqDivider->setSlow(4);

			m_pFlimLaserFreqDivider->SendStatusMessage += SendStatusMessage;
		}
		else
			return true;

#ifndef NEXT_GEN_SYSTEM
		// Create Axsun OCT sync control objects
		if (!m_pAxsunFreqDivider)
		{
			m_pAxsunFreqDivider = new FreqDivider;
			m_pAxsunFreqDivider->setSourceTerminal(AXSUN_SOURCE_TERMINAL);
			m_pAxsunFreqDivider->setCounterChannel(AXSUN_COUNTER_CHANNEL);
			m_pAxsunFreqDivider->setSlow(OCT_ALINES);

			m_pAxsunFreqDivider->SendStatusMessage += SendStatusMessage;
		}
		else
			return true;
#else
		// Create FLIm DAQ sync control objects
		if (!m_pFlimDaqFreqDivider)
		{
			m_pFlimDaqFreqDivider = new FreqDivider;
			m_pFlimDaqFreqDivider->setSourceTerminal(!async ? FLIM_DAQ_SOURCE_TERMINAL : "100kHzTimebase");
			m_pFlimDaqFreqDivider->setCounterChannel(FLIM_DAQ_COUNTER_CHANNEL);
			m_pFlimDaqFreqDivider->setSlow(4);

			m_pFlimDaqFreqDivider->SendStatusMessage += SendStatusMessage;
		}
		else
			return false;
#endif

		// Initializing
#ifndef NEXT_GEN_SYSTEM
		if (m_pFlimLaserFreqDivider->initialize() && m_pAxsunFreqDivider->initialize())
		{
			// Generate FLIm laser & Axsun OCT sync
			m_pFlimLaserFreqDivider->start();
			m_pAxsunFreqDivider->start();
#else
		if (m_pFlimLaserFreqDivider->initialize() && m_pFlimDaqFreqDivider->initialize())
		{
			// Generate FLIm laser sync
			m_pFlimLaserFreqDivider->start();
			m_pFlimDaqFreqDivider->start();
#endif
			char msg[256];
            snprintf(msg, 256, "[SYNC] Synchronization started. (source: %s)", !async ? FLIM_LASER_SOURCE_TERMINAL : "100kHzTimebase");
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
		if (m_pFlimLaserFreqDivider)
		{
			m_pFlimLaserFreqDivider->stop();
			delete m_pFlimLaserFreqDivider;
			m_pFlimLaserFreqDivider = nullptr;
		}

#ifndef NEXT_GEN_SYSTEM
		// Delete Axsun OCT sync control objects
		if (m_pAxsunFreqDivider)
		{
			m_pAxsunFreqDivider->stop();
			delete m_pAxsunFreqDivider;
			m_pAxsunFreqDivider = nullptr;
		}
#else
		// Delete FLIm DAQ sync control objects
		if (m_pFlimDaqFreqDivider)
		{
			m_pFlimDaqFreqDivider->stop();
			delete m_pFlimDaqFreqDivider;
			m_pFlimDaqFreqDivider = nullptr;
		}
#endif
	}
#else
	(void)enabled;
	(void)async;
#endif

	return true;
}


bool DeviceControl::connectAxsunControl(bool toggled)
{
#ifdef AXSUN_ENABLE
	if (toggled)
	{
		// Create Axsun OCT control objects
		if (!m_pAxsunControl)
		{
			m_pAxsunControl = new AxsunControl;
			m_pAxsunControl->SendStatusMessage += SendStatusMessage;
		}
		else
			return true;
		
		// Initialize the Axsun OCT control
		if (!(m_pAxsunControl->initialize()))
		{
			connectAxsunControl(false);
            return false;
		}
		
#ifndef NEXT_GEN_SYSTEM
		// Default Bypass Mode
		setPipelineMode(m_pConfig->axsunPipelineMode);
#endif

		// Default Clock Delay
		setClockDelay(CLOCK_GAIN * CLOCK_DELAY + CLOCK_OFFSET);

		// Default VDL Length		
		m_pAxsunControl->getVDLStatus();
		if (m_pAxsunControl->last_move_time == 0)
			setVDLHome();
		
		///setVDLHome();
		///setVDLLength(m_pConfig->axsunVDLLength);
		///m_pAxsunControl->setVDLHome();
		///m_pAxsunControl->setVDLLength(m_pConfig->axsunVDLLength);
///		std::thread vdl_length([&]() {
///			//std::this_thread::sleep_for(std::chrono::milliseconds(1500));
///			//m_pAxsunControl->setVDLLength(m_pConfig->axsunVDLLength);
///
///		//std::this_thread::sleep_for(std::chrono::milliseconds(2000));
///				if (m_pStreamTab->getOperationTab()->getAcquisitionButton()->isChecked())
///					setVDLWidgets(true);
///		});
///		vdl_length.detach();

		// Dispersion Compensation(+ set windowing function)
		m_pAxsunControl->setDispersionCompensation(m_pConfig->axsunDispComp_a2, m_pConfig->axsunDispComp_a3);

#ifndef NEXT_GEN_SYSTEM
		// Default Contrast Range
		adjustDecibelRange(m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max);
#endif
	}
	else
	{				
		if (m_pAxsunControl)
		{
			// Set status
			if (m_pAxsunControl->isInitialized())
			{
#ifndef NEXT_GEN_SYSTEM
				setLiveImaging(false);
#endif
				setLightSource(false);
			}

			// Delete Axsun OCT control objects
			delete m_pAxsunControl;
			m_pAxsunControl = nullptr;
		}
	}
#else
	(void)toggled;
#endif

    return true;
}

void DeviceControl::setLightSource(bool toggled)
{
#ifdef AXSUN_ENABLE
	if (m_pAxsunControl)
	{
		// Start or stop Axsun light source operation
		if (m_pAxsunControl->isInitialized())
			m_pAxsunControl->setLaserEmission(toggled);		
	}
#else
	(void)toggled;
#endif
}

void DeviceControl::setLiveImaging(bool toggled)
{
#ifdef AXSUN_ENABLE
#ifndef NEXT_GEN_SYSTEM
	if (m_pAxsunControl)
	{
		// Start or stop Axsun live imaging operation
		if (m_pAxsunControl->isInitialized())
			m_pAxsunControl->setLiveImagingMode(toggled);	
	}
#else
	(void)toggled;
#endif
#else
	(void)toggled;
#endif
}

void DeviceControl::adjustDecibelRange(double min, double max)
{
#ifdef AXSUN_ENABLE
#ifndef NEXT_GEN_SYSTEM
	if (m_pAxsunControl)
	{
		m_pConfig->axsunDbRange.min = min;
		m_pConfig->axsunDbRange.max = max;
		m_pAxsunControl->setDecibelRange(m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max);
	}
#else
	m_pConfig->axsunDbRange.min = min;
	m_pConfig->axsunDbRange.max = max;
#endif
#else
	(void)min;
	(void)max;
#endif
}

void DeviceControl::setBackground()
{
#ifdef AXSUN_ENABLE
	if (m_pAxsunControl)
		m_pAxsunControl->setBackground();
#endif
}

void DeviceControl::setDispersionCompensation(float a2, float a3)
{
#ifdef AXSUN_ENABLE
	if (m_pAxsunControl)
	{
		m_pConfig->axsunDispComp_a2 = a2;
		m_pConfig->axsunDispComp_a3 = a3;
		m_pAxsunControl->setDispersionCompensation(a2, a3);
	}
#else
	(void)a2;
	(void)a3;
#endif
}

void DeviceControl::setClockDelay(double)
{
#ifdef AXSUN_ENABLE
	if (m_pAxsunControl)
		m_pAxsunControl->setClockDelay(CLOCK_DELAY);
#endif
}

void DeviceControl::setSubSampling(int M)
{
#ifdef AXSUN_ENABLE
	if (m_pAxsunControl)
		m_pAxsunControl->setSubSampling(M);
#else
	(void)M;
#endif
}

void DeviceControl::setPipelineMode(int mode)
{	
#ifdef AXSUN_ENABLE
	if (m_pAxsunControl)
	{
		m_pConfig->axsunPipelineMode = mode;
		if (mode == 0)
		{
			m_pAxsunControl->setPipelineMode(AxPipelineMode::JPEG_COMP);
			m_pAxsunControl->setSubSampling(1);
		}
		else
		{
			m_pAxsunControl->setPipelineMode(AxPipelineMode::RAW_ADC);
			m_pAxsunControl->setSubSampling(16);
		}
	}
#else
	(void)mode;
#endif
}

void DeviceControl::setVDLLength(double length)
{
#ifdef AXSUN_ENABLE
	if (m_pAxsunControl)
	{
		m_pConfig->axsunVDLLength = length;
		m_pAxsunControl->setVDLLength(length);
	}
#else
	(void)length;
#endif
}

void DeviceControl::setVDLHome()
{
#ifdef AXSUN_ENABLE
	if (m_pAxsunControl)
	{
		m_pConfig->axsunVDLLength = 0.0f;
		m_pAxsunControl->setVDLHome();
	}
#endif
}

void DeviceControl::requestOctStatus()
{
	///if (m_pAxsunControl) 
	///	if (m_pAxsunControl->isInitialized())
	///		m_pAxsunControl->getDeviceState();
}
