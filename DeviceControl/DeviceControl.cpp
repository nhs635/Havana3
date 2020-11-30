
#include "DeviceControl.h"

#include <Havana3/Configuration.h>

#include <DeviceControl/FreqDivider/FreqDivider.h>
#include <DeviceControl/PmtGainControl/PmtGainControl.h>
#include <DeviceControl/ElforlightLaser/ElforlightLaser.h>
#include <DeviceControl/AxsunControl/AxsunControl.h>
#include <DeviceControl/FaulhaberMotor/PullbackMotor.h>
#include <DeviceControl/FaulhaberMotor/RotaryMotor.h>

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
	m_pFlimFreqDivider(nullptr), m_pAxsunFreqDivider(nullptr), m_pPmtGainControl(nullptr),
    m_pElforlightLaser(nullptr), m_pAxsunControl(nullptr)
{

}

DeviceControl::~DeviceControl()
{
	//setControlsStatus();
}


void DeviceControl::setControlsStatus()
{
//	if (m_pGroupBox_HelicalScanningControl->isChecked()) m_pGroupBox_HelicalScanningControl->setChecked(false);
//	if (m_pGroupBox_FlimControl->isChecked()) m_pGroupBox_FlimControl->setChecked(false);
//	if (m_pGroupBox_AxsunOctControl->isChecked()) m_pGroupBox_AxsunOctControl->setChecked(false);
}


bool DeviceControl::connectPullbackMotor(bool enabled)
{
    if (enabled)
	{
		// Create Faulhaber pullback motor control objects
		if (!m_pPullbackMotor)
		{
			m_pPullbackMotor = new PullbackMotor;
//			m_pPullbackMotor->SendStatusMessage += [&](const char* msg, bool is_error) {
//				QString qmsg = QString::fromUtf8(msg);
//				qmsg.replace('\n', ' ');
////				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
//			};
		}

		// Connect the motor
		if (!(m_pPullbackMotor->ConnectDevice()))
		{
			m_pPullbackMotor->SendStatusMessage("Faulhaber pullback motor is not connected!", true);

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
	// Pullback
    m_pPullbackMotor->RotateMotor(-int(m_pConfig->pullbackSpeed * GEAR_RATIO));

	// Pullback end condition
    float duration = m_pConfig->pullbackLength / m_pConfig->pullbackSpeed;
	std::thread stop([&, duration]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(int(1000 * 2)));// duration)));
		this->stop();
	});
	stop.detach();
}

void DeviceControl::setTargetSpeed(float speed)
{
    m_pConfig->pullbackSpeed = speed;
}

void DeviceControl::changePullbackLength(float length)
{
    m_pConfig->pullbackLength = length;
}

void DeviceControl::home()
{
    m_pPullbackMotor->RotateMotor(int(20 * GEAR_RATIO));
}

void DeviceControl::stop()
{
	// Stop pullback motor
	m_pPullbackMotor->StopMotor();

//	// Stop rotary motor
//	if (m_pToggleButton_Rotate->isChecked())
//	{
//		m_pToggleButton_Rotate->setChecked(false);

//		m_pGroupBox_HelicalScanningControl->setChecked(false);
//		m_pGroupBox_HelicalScanningControl->setChecked(true);
//	}

//	// Set widgets
//	m_pLabel_PullbackSpeed->setEnabled(true);
//	m_pLabel_PullbackSpeedUnit->setEnabled(true);
//	m_pLineEdit_PullbackSpeed->setEnabled(true);

//	m_pLabel_PullbackLength->setEnabled(true);
//	m_pLabel_PullbackLengthUnit->setEnabled(true);
//	m_pLineEdit_PullbackLength->setEnabled(true);

//	m_pPushButton_Home->setEnabled(true);

//	m_pPushButton_Pullback->setStyleSheet("QPushButton { background-color:#ff0000; }");
}


bool DeviceControl::connectRotaryMotor(bool toggled)
{
	if (toggled)
	{
		// Create Faulhaber rotary motor control objects
		if (!m_pRotaryMotor)
		{
			m_pRotaryMotor = new RotaryMotor;
//			m_pRotaryMotor->SendStatusMessage += [&](const char* msg, bool is_error) {
//				QString qmsg = QString::fromUtf8(msg);
//				qmsg.replace('\n', ' ');
//				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
//			};
		}

		// Connect the motor
		if (!(m_pRotaryMotor->ConnectDevice()))
		{
			m_pRotaryMotor->SendStatusMessage("Faulhaber rotary motor is not connected!", true);

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
	m_pRotaryMotor->RotateMotor(toggled ? m_pConfig->rotaryRpm : 0);
	
//	m_pLineEdit_RPM->setDisabled(toggled);
//	m_pLabel_RPM->setDisabled(toggled);

//	m_pToggleButton_Rotate->setStyleSheet(toggled ? "QPushButton { background-color:#00ff00; }" : "QPushButton { background-color:#ff0000; }");
//	m_pToggleButton_Rotate->setText(toggled ? "Stop" : "Rotate");
}

void DeviceControl::changeRotaryRpm(int rpm)
{
    m_pConfig->rotaryRpm = rpm;
}

//void DeviceControl::setHelicalScanningControl(bool toggled)
//{
//	// Set Helical Scanning Control
//	m_pGroupBox_HelicalScanningControl->setChecked(toggled);
//}


//void DeviceControl::initializeFlimSystem(bool toggled)
//{
//    if (toggled)
//    {
//		// DAQ Board Connection Check
//		if (isDaqBoardConnected())
//		{
//			// Set enabled true for FLIm control widgets
//			if (!m_pToggleButton_SynchronizedPulsedLaser->isChecked())
//			{
//				m_pLabel_AsynchronizedPulsedLaser->setEnabled(true);
//				m_pToggleButton_AsynchronizedPulsedLaser->setEnabled(true);
//			}
//			if (!m_pToggleButton_AsynchronizedPulsedLaser->isChecked())
//			{
//				m_pLabel_SynchronizedPulsedLaser->setEnabled(true);
//				m_pToggleButton_SynchronizedPulsedLaser->setEnabled(true);
//			}
//			m_pLabel_PmtGainControl->setEnabled(true);
//			m_pLineEdit_PmtGainVoltage->setEnabled(true);
//			m_pLabel_PmtGainVoltage->setEnabled(true);
//			m_pToggleButton_PmtGainVoltage->setEnabled(true);

//			if (!m_pStreamTab->getOperationTab()->m_pAcquisitionState)
//			{
//				m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");
//				m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");
//				m_pToggleButton_PmtGainVoltage->setStyleSheet("QPushButton { background-color:#ff0000; }");
//			}
//		}
//		else
//		{
//			m_pGroupBox_FlimControl->setChecked(false);
//			return;
//		}

//		// FLIm Laser Connection Check
//		if (connectFlimLaser(true))
//		{
//			// Set enabled true for FLIM laser power control widgets
//			m_pLabel_FlimLaserPowerControl->setEnabled(true);
//			m_pSpinBox_FlimLaserPowerControl->setEnabled(true);
//		}
//		else
//		{
//			m_pGroupBox_FlimControl->setChecked(false);
//			return;
//		}

//		// Digitizer Connection Check
//		if (m_pStreamTab->getOperationTab()->getDataAcq()->getDigitizer()->is_initialized())
//		{
//			// Set enabled true for FLIm pulse view and calibration button
//			m_pPushButton_FlimCalibDlgration->setEnabled(true);
//		}
//		///else
//		///{
//		///	m_pGroupBox_FlimControl->setChecked(false);
//		///	return;
//		///}
//    }
//    else
//    {
//		// If acquisition is processing...
//		if (m_pStreamTab->getOperationTab()->m_pAcquisitionState)
//		{
//			QMessageBox MsgBox;
//			MsgBox.setWindowTitle("Warning");
//			MsgBox.setIcon(QMessageBox::Warning);
//			MsgBox.setText("Re-turning the FLIm system on does not guarantee the synchronized operation once you turn off the laser.\nWould you like to turn off the FLIm system?");
//			MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
//			MsgBox.setDefaultButton(QMessageBox::No);

//			int resp = MsgBox.exec();
//			switch (resp)
//			{
//			case QMessageBox::Yes:
//				m_pStreamTab->getOperationTab()->setAcquisitionButton(false);
//				break;
//			case QMessageBox::No:
//				m_pGroupBox_FlimControl->setChecked(true);
//				return;
//			default:
//				m_pGroupBox_FlimControl->setChecked(true);
//				return;
//			}
//		}
		
//		// Set buttons
//		if (m_pToggleButton_AsynchronizedPulsedLaser->isChecked()) m_pToggleButton_AsynchronizedPulsedLaser->setChecked(false);
//		if (m_pToggleButton_SynchronizedPulsedLaser->isChecked()) m_pToggleButton_SynchronizedPulsedLaser->setChecked(false);
//		if (m_pToggleButton_PmtGainVoltage->isChecked()) m_pToggleButton_PmtGainVoltage->setChecked(false);
//		m_pToggleButton_SynchronizedPulsedLaser->setText("On");

//		m_pLabel_AsynchronizedPulsedLaser->setEnabled(false);
//		m_pToggleButton_AsynchronizedPulsedLaser->setEnabled(false);
//		m_pLabel_SynchronizedPulsedLaser->setEnabled(false);
//		m_pToggleButton_SynchronizedPulsedLaser->setEnabled(false);
//		m_pLabel_PmtGainControl->setEnabled(false);
//		m_pLineEdit_PmtGainVoltage->setEnabled(false);
//		m_pLabel_PmtGainVoltage->setEnabled(false);
//		m_pToggleButton_PmtGainVoltage->setEnabled(false);
//		m_pLabel_FlimLaserPowerControl->setEnabled(false);
//		m_pSpinBox_FlimLaserPowerControl->setEnabled(false);
//		m_pPushButton_FlimCalibDlgration->setEnabled(false);

//		m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#353535; }");
//		m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#353535; }");
//		m_pToggleButton_PmtGainVoltage->setStyleSheet("QPushButton { background-color:#353535; }");

//		// Disconnect devices
//		connectFlimLaser(false);
//    }
//}

//bool DeviceControl::isDaqBoardConnected()
//{
//#ifdef NI_ENABLE
//	if (!m_pPmtGainControl)
//	{
//		// Create temporary PMT gain control objects
//		PmtGainControl *pTempGain = new PmtGainControl;
//		pTempGain->SendStatusMessage += [&](const char* msg, bool is_error) {
//			QString qmsg = QString::fromUtf8(msg);
//			emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
//		};

//		pTempGain->voltage = 0;
//		if (!pTempGain->initialize())
//		{
//			pTempGain->SendStatusMessage("DAQ Board is not connected!", true);
//			delete pTempGain;
//			return false;
//		}

//		pTempGain->stop();
//		delete pTempGain;
//	}

//	return true;
//#else
//	return false;
//#endif
//}

//void DeviceControl::startFlimAsynchronization(bool toggled)
//{
//#ifdef NI_ENABLE
//	if (toggled)
//	{
//		// Set text
//		m_pToggleButton_AsynchronizedPulsedLaser->setText("Off");
//		m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#00ff00; }");

//		// Create FLIm laser async control objects
//		if (!m_pFlimFreqDivider)
//		{
//			m_pFlimFreqDivider = new FreqDivider;
//			m_pFlimFreqDivider->SendStatusMessage += [&](const char* msg, bool is_error) {
//				QString qmsg = QString::fromUtf8(msg);
//				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
//			};
//		}
//		m_pFlimFreqDivider->sourceTerminal = "100kHzTimebase";
//		m_pFlimFreqDivider->counterChannel = NI_FLIM_TRIG_CHANNEL;
//		m_pFlimFreqDivider->slow = 4;

//		// Initializing
//		if (!m_pFlimFreqDivider->initialize())
//		{
//			m_pToggleButton_AsynchronizedPulsedLaser->setChecked(false);
//			return;
//		}

//		// Generate FLIm laser
//		m_pFlimFreqDivider->start();

//		// Set widgets
//		m_pLabel_SynchronizedPulsedLaser->setEnabled(false);
//		m_pToggleButton_SynchronizedPulsedLaser->setEnabled(false);
//		m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#353535; }");
//	}
//	else
//	{
//		// Delete FLIm laser async control objects
//		if (m_pFlimFreqDivider)
//		{
//			m_pFlimFreqDivider->stop();
//			delete m_pFlimFreqDivider;
//			m_pFlimFreqDivider = nullptr;
//		}

//		// Set text
//		m_pToggleButton_AsynchronizedPulsedLaser->setText("On");
//		m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");

//		// Set widgets
//		m_pLabel_SynchronizedPulsedLaser->setEnabled(true);
//		m_pToggleButton_SynchronizedPulsedLaser->setEnabled(true);
//		m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");
//	}
//#else
//	(void)toggled;
//#endif
//}

bool DeviceControl::startSynchronization(bool enabled)
{
#ifdef NI_ENABLE
    if (enabled)
	{
		// Create FLIm laser sync control objects
		if (!m_pFlimFreqDivider)
		{
			m_pFlimFreqDivider = new FreqDivider;
//			m_pFlimFreqDivider->SendStatusMessage += [&](const char* msg, bool is_error) {
//				QString qmsg = QString::fromUtf8(msg);
//				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
//			};
		}
		m_pFlimFreqDivider->sourceTerminal = NI_FLIM_TRIG_SOURCE;
		m_pFlimFreqDivider->counterChannel = NI_FLIM_TRIG_CHANNEL;
		m_pFlimFreqDivider->slow = 4;

		// Create Axsun OCT sync control objects
		if (!m_pAxsunFreqDivider)
		{
			m_pAxsunFreqDivider = new FreqDivider;
//			m_pAxsunFreqDivider->SendStatusMessage += [&](const char* msg, bool is_error) {
//				QString qmsg = QString::fromUtf8(msg);
//				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
//			};
		}
		m_pAxsunFreqDivider->sourceTerminal = NI_AXSUN_TRIG_SOURCE;
		m_pAxsunFreqDivider->counterChannel = NI_AXSUN_TRIG_CHANNEL;
		m_pAxsunFreqDivider->slow = 1024;

		// Initializing
		if (!m_pFlimFreqDivider->initialize() || !m_pAxsunFreqDivider->initialize())
		{
//			m_pToggleButton_SynchronizedPulsedLaser->setChecked(false);
            return false;
		}

		// Generate FLIm laser & Axsun OCT sync
		m_pFlimFreqDivider->start();
		m_pAxsunFreqDivider->start();

		// Set widgets
//		m_pLabel_AsynchronizedPulsedLaser->setEnabled(false);
//		m_pToggleButton_AsynchronizedPulsedLaser->setEnabled(false);
//		m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#353535; }");
	}
	else
	{
		// If acquisition is processing...
//		if (m_pStreamTab->getOperationTab()->m_pAcquisitionState)
//		{
//			QMessageBox MsgBox;
//			MsgBox.setWindowTitle("Warning");
//			MsgBox.setIcon(QMessageBox::Warning);
//			MsgBox.setText("Re-turning the laser on does not guarantee the synchronized operation once you turn off the laser.\nWould you like to turn off the laser?");
//			MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
//			MsgBox.setDefaultButton(QMessageBox::No);

//			int resp = MsgBox.exec();
//			switch (resp)
//			{
//				case QMessageBox::Yes:
//					m_pStreamTab->getOperationTab()->setAcquisitionButton(false);
//					break;
//				case QMessageBox::No:
//					m_pToggleButton_SynchronizedPulsedLaser->setChecked(true);
//					return;
//				default:
//					m_pToggleButton_SynchronizedPulsedLaser->setChecked(true);
//					return;
//			}
//		}
//		else
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

//			// Set text
//			m_pToggleButton_SynchronizedPulsedLaser->setText("On");
//			m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");

//			// Set widgets
//			m_pLabel_AsynchronizedPulsedLaser->setEnabled(true);
//			m_pToggleButton_AsynchronizedPulsedLaser->setEnabled(true);
//			m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");
		}
	}
#else
    (void)enabled;
#endif

    return true;
}

bool DeviceControl::applyPmtGainVoltage(bool enabled)
{
#ifdef NI_ENABLE
	// Set text
//	m_pToggleButton_PmtGainVoltage->setText(toggled ? "Off" : "On");
//	m_pToggleButton_PmtGainVoltage->setStyleSheet(toggled ? "QPushButton { background-color:#00ff00; }" : "QPushButton { background-color:#ff0000; }");
	
    if (enabled)
	{
		// Set enabled false for PMT gain control widgets
//		m_pLineEdit_PmtGainVoltage->setEnabled(false);
//		m_pLabel_PmtGainVoltage->setEnabled(false);

		// Create PMT gain control objects
		if (!m_pPmtGainControl)
		{
			m_pPmtGainControl = new PmtGainControl;
//			m_pPmtGainControl->SendStatusMessage += [&](const char* msg, bool is_error) {
//				QString qmsg = QString::fromUtf8(msg);
//				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
//			};
		}

//		m_pPmtGainControl->voltage = m_pLineEdit_PmtGainVoltage->text().toDouble();
		if (m_pPmtGainControl->voltage > 1.0)
		{
			m_pPmtGainControl->SendStatusMessage(">1.0V Gain cannot be assigned!", true);
//			m_pToggleButton_PmtGainVoltage->setChecked(false);
            return false;
		}

		// Initializing
		if (!m_pPmtGainControl->initialize())
		{
//			m_pToggleButton_PmtGainVoltage->setChecked(false);
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

		// Set enabled true for PMT gain control widgets
//		m_pLineEdit_PmtGainVoltage->setEnabled(true);
//		m_pLabel_PmtGainVoltage->setEnabled(true);
	}	
#else
    (void)enabled;
#endif

    return true;
}

void DeviceControl::changePmtGainVoltage(float voltage)
{
    m_pConfig->pmtGainVoltage = voltage;
}

bool DeviceControl::connectFlimLaser(bool state)
{
	if (state)
	{
		// Create FLIM laser power control objects
		if (!m_pElforlightLaser)
		{
			m_pElforlightLaser = new ElforlightLaser;
//			m_pElforlightLaser->SendStatusMessage += [&](const char* msg, bool is_error) {
//				QString qmsg = QString::fromUtf8(msg);
//				qmsg.replace('\n', ' ');
//				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
//			};
//			m_pElforlightLaser->UpdateState += [&](double* state) {
////				m_pStreamTab->getMainWnd()->updateFlimLaserState(state);
//			};
		}

		// Connect the laser
		if (!(m_pElforlightLaser->ConnectDevice()))
		{
			m_pElforlightLaser->SendStatusMessage("UV Pulsed Laser is not connected!", true);

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
	static int i = 0;

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
}

void DeviceControl::sendLaserCommand(char* command)
{
	if (m_pElforlightLaser) 
		//if (m_pElforlightLaser->isLaserEnabled)
		m_pElforlightLaser->SendCommand(command);
}

//void DeviceControl::createFlimCalibDlgDlg()
//{
//    if (m_pFlimCalibDlgDlg == nullptr)
//    {
//        m_pFlimCalibDlgDlg = new FlimCalibDlgDlg(this);
//        connect(m_pFlimCalibDlgDlg, SIGNAL(finished(int)), this, SLOT(deleteFlimCalibDlgDlg()));
//		m_pFlimCalibDlgDlg->SendStatusMessage += [&](const char* msg) {
//			QString qmsg = QString::fromUtf8(msg);
//			emit m_pStreamTab->sendStatusMessage(qmsg, false);
//		};
//        m_pFlimCalibDlgDlg->show();
/////        emit m_pFlimCalibDlgDlg->plotRoiPulse(m_pFLIm, m_pSlider_SelectAline->value() / 4);
//    }
//    m_pFlimCalibDlgDlg->raise();
//    m_pFlimCalibDlgDlg->activateWindow();
//}

//void DeviceControl::deleteFlimCalibDlgDlg()
//{
/////    m_pFlimCalibDlgDlg->showWindow(false);
/////    m_pFlimCalibDlgDlg->showMeanDelay(false);
/////    m_pFlimCalibDlgDlg->showMask(false);

//    m_pFlimCalibDlgDlg->deleteLater();
//    m_pFlimCalibDlgDlg = nullptr;
//}

//void DeviceControl::setFlimControl(bool toggled)
//{
//	// Set FLIm Control
//	if (toggled)
//	{
//		if (m_pGroupBox_FlimControl->isChecked()) m_pGroupBox_FlimControl->setChecked(false);
//		m_pGroupBox_FlimControl->setChecked(toggled);
//		if (m_pElforlightLaser)
//		{
//			m_pToggleButton_SynchronizedPulsedLaser->setChecked(toggled);
//			m_pToggleButton_PmtGainVoltage->setChecked(toggled);
//		}
//	}
//	else
//		m_pGroupBox_FlimControl->setChecked(toggled);
//}


bool DeviceControl::connectAxsunControl(bool toggled)
{
	if (toggled)
	{
		// Create Axsun OCT control objects
		if (!m_pAxsunControl)
		{
			m_pAxsunControl = new AxsunControl;
//			m_pAxsunControl->SendStatusMessage += [&](const char* msg, bool is_error) {
//				QString qmsg = QString::fromUtf8(msg);
//				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
//				if (is_error)
//				{
//					m_pStreamTab->getOperationTab()->getProgressBar()->setFormat("");
//					m_pStreamTab->getOperationTab()->getProgressBar()->setValue(0);
//				}
//			};
//			m_pAxsunControl->DidTransferArray += [&](int i) {
//				emit transferAxsunArray(i);
//			};
			
			// Initialize the Axsun OCT control
			if (!(m_pAxsunControl->initialize()))
			{
//				m_pGroupBox_AxsunOctControl->setChecked(false);
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
			adjustDecibelRange();
		}
				
//		// Set enabled true for Axsun OCT control widgets
//		m_pLabel_LightSource->setEnabled(true);
//		m_pLabel_LiveImaging->setEnabled(true);
//		m_pToggleButton_LightSource->setEnabled(true);
//		m_pToggleButton_LiveImaging->setEnabled(true);

//		if (!m_pStreamTab->getOperationTab()->m_pAcquisitionState)
//		{
//			m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#ff0000; }");
//			m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#ff0000; }");
//		}
		
//		m_pStreamTab->getVisTab()->setOctDecibelContrastWidgets(true);
	}
	else
	{
//		// If acquisition is processing...
//		if (m_pStreamTab->getOperationTab()->m_pAcquisitionState)
//		{
//			QMessageBox MsgBox;
//			MsgBox.setWindowTitle("Warning");
//			MsgBox.setIcon(QMessageBox::Warning);
//			MsgBox.setText("Re-turning the Axsun OCT system on does not guarantee the synchronized operation once you turn off the laser.\nWould you like to turn off the Axsun OCT system?");
//			MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
//			MsgBox.setDefaultButton(QMessageBox::No);

//			int resp = MsgBox.exec();
//			switch (resp)
//			{
//			case QMessageBox::Yes:
//				m_pStreamTab->getOperationTab()->setAcquisitionButton(false);
//				break;
//			case QMessageBox::No:
//				m_pGroupBox_AxsunOctControl->setChecked(true);
//				return;
//			default:
//				m_pGroupBox_AxsunOctControl->setChecked(true);
//				return;
//			}
//		}

//		// Set buttons
//		if (m_pToggleButton_LightSource->isChecked()) m_pToggleButton_LightSource->setChecked(false);
//		if (m_pToggleButton_LiveImaging->isChecked()) m_pToggleButton_LiveImaging->setChecked(false);
//		m_pToggleButton_LightSource->setText("On");
//		m_pToggleButton_LiveImaging->setText("On");

//		// Set disabled true for Axsun OCT control widgets
//		m_pLabel_LightSource->setDisabled(true);
//		m_pLabel_LiveImaging->setDisabled(true);
//		m_pToggleButton_LightSource->setDisabled(true);
//		m_pToggleButton_LiveImaging->setDisabled(true);
//		m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#353535; }");
//		m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#353535; }");
		
//		//m_pLabel_VDLLength->setDisabled(true);
//		//m_pSpinBox_VDLLength->setDisabled(true);
//		//m_pPushButton_VDLHome->setDisabled(true);

//		m_pStreamTab->getVisTab()->setOctDecibelContrastWidgets(false);

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
		if (toggled)
		{
//			 Set text
//			m_pToggleButton_LightSource->setText("Off");
//			m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#00ff00; }");

			// Start Axsun light source operation
			m_pAxsunControl->setLaserEmission(true);
		}
		else
		{
			// If acquisition is processing...
//			if (m_pStreamTab->getOperationTab()->m_pAcquisitionState)
//			{
//				QMessageBox MsgBox;
//				MsgBox.setWindowTitle("Warning");
//				MsgBox.setIcon(QMessageBox::Warning);
//				MsgBox.setText("Re-turning the laser on does not guarantee the synchronized operation once you turn off the laser.\nWould you like to turn off the laser?");
//				MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
//				MsgBox.setDefaultButton(QMessageBox::No);

//				int resp = MsgBox.exec();
//				switch (resp)
//				{
//				case QMessageBox::Yes:
//					m_pStreamTab->getOperationTab()->setAcquisitionButton(false);
//					break;
//				case QMessageBox::No:
//					m_pToggleButton_LightSource->setChecked(true);
//					return;
//				default:
//					m_pToggleButton_LightSource->setChecked(true);
//					return;
//				}
//			}
//			else
			{
				// Stop Axsun light source operation
				if (m_pAxsunControl) m_pAxsunControl->setLaserEmission(false);

				// Set text
//				m_pToggleButton_LightSource->setText("On");
//				m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#ff0000; }");
			}
		}
	}
}

void DeviceControl::setLiveImaging(bool toggled)
{
	if (m_pAxsunControl)
	{
		if (toggled)
		{
			// Set text and widgets
//			m_pToggleButton_LiveImaging->setText("Off");
//			m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#00ff00; }");

			// Start Axsun live imaging operation
			m_pAxsunControl->setLiveImagingMode(true);
		}
		else
		{
//			// If acquisition is processing...
//			if (m_pStreamTab->getOperationTab()->m_pAcquisitionState)
//			{
//				QMessageBox MsgBox;
//				MsgBox.setWindowTitle("Warning");
//				MsgBox.setIcon(QMessageBox::Warning);
//				MsgBox.setText("Re-turning the live imaging mode on does not guarantee the synchronized operation once you turn off the live imaging mode.\nWould you like to turn off the live imaging mode?");
//				MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
//				MsgBox.setDefaultButton(QMessageBox::No);

//				int resp = MsgBox.exec();
//				switch (resp)
//				{
//				case QMessageBox::Yes:
//					m_pStreamTab->getOperationTab()->setAcquisitionButton(false);
//					break;
//				case QMessageBox::No:
//					m_pToggleButton_LiveImaging->setChecked(true);
//					return;
//				default:
//					m_pToggleButton_LiveImaging->setChecked(true);
//					return;
//				}
//			}
//			else
			{
				// Stop Axsun live imaging operation
				if (m_pAxsunControl) m_pAxsunControl->setLiveImagingMode(false);

				// Set text and widgets
//				m_pToggleButton_LiveImaging->setText("On");
//				m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#ff0000; }");
			}
		}
	}
}

void DeviceControl::adjustDecibelRange()
{
	if (m_pAxsunControl)
		m_pAxsunControl->setDecibelRange(m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max);
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
		m_pAxsunControl->setVDLHome();
//		m_pSpinBox_VDLLength->setValue(0.0);
	}
}

//void DeviceControl::setVDLWidgets(bool enabled)
//{
//	m_pLabel_VDLLength->setEnabled(enabled);
//	m_pSpinBox_VDLLength->setEnabled(enabled);
//	m_pPushButton_VDLHome->setEnabled(enabled);
//}

//void DeviceControl::setAxsunControl(bool toggled)
//{
//	// Set Axsun OCT Control
//	if (toggled)
//	{
//		if (m_pGroupBox_AxsunOctControl->isChecked()) m_pGroupBox_AxsunOctControl->setChecked(false);
//		m_pGroupBox_AxsunOctControl->setChecked(toggled);
//		if (m_pAxsunControl)
//		{
//			m_pToggleButton_LightSource->setChecked(toggled);
//			m_pToggleButton_LiveImaging->setChecked(toggled);
//		}
//	}
//	else
//		m_pGroupBox_AxsunOctControl->setChecked(toggled);
//}

void DeviceControl::requestOctStatus()
{
	if (m_pAxsunControl) m_pAxsunControl->getDeviceState();
}
