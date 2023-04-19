
#include "DeviceOptionTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QResultTab.h>

#include <Havana3/Dialog/SettingDlg.h>

#include <DeviceControl/DeviceControl.h>
#include <DeviceControl/FaulhaberMotor/RotaryMotor.h>
#include <DeviceControl/FaulhaberMotor/PullbackMotor.h>
#ifdef NI_ENABLE
#include <DeviceControl/PmtGainControl/PmtGainControl.h>
#endif
#include <DeviceControl/ElforlightLaser/ElforlightLaser.h>
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
#include <vector>
#include <chrono>


DeviceOptionTab::DeviceOptionTab(QWidget *parent) :
    QDialog(parent), m_pPatientSummaryTab(nullptr), m_pStreamTab(nullptr), m_pResultTab(nullptr), m_pDeviceControl(nullptr)
{
    // Set configuration objects
	QString parent_name, parent_title = parent->windowTitle();
	if (parent_title.contains("Summary"))
	{
		parent_name = "Summary";
		m_pPatientSummaryTab = dynamic_cast<QPatientSummaryTab*>(parent);
		m_pStreamTab = m_pPatientSummaryTab->getMainWnd()->getStreamTab();
		m_pConfig = m_pPatientSummaryTab->getMainWnd()->m_pConfiguration;		
		if (!m_pStreamTab)
			m_pDeviceControl = new DeviceControl(m_pConfig);
		else
			m_pDeviceControl = m_pStreamTab->getDeviceControl();
	}
	else if (parent_title.contains("Streaming"))
	{
		parent_name = "Streaming";
		m_pStreamTab = dynamic_cast<QStreamTab*>(parent);
		m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;
		m_pDeviceControl = m_pStreamTab->getDeviceControl();
	}
	else if (parent_title.contains("Review"))
	{
		parent_name = "Review";
		m_pResultTab = dynamic_cast<QResultTab*>(parent);
		m_pStreamTab = m_pResultTab->getMainWnd()->getStreamTab();
		m_pConfig = m_pResultTab->getMainWnd()->m_pConfiguration;
		if (!m_pStreamTab)
			m_pDeviceControl = new DeviceControl(m_pConfig);
		else
			m_pDeviceControl = m_pStreamTab->getDeviceControl();
	}	

	// Create layout
	m_pGroupBox_DeviceOption = new QGroupBox;
	m_pGroupBox_DeviceOption->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_DeviceOption->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

	m_pVBoxLayout_DeviceOption = new QVBoxLayout;
	m_pVBoxLayout_DeviceOption->setSpacing(3);
	
    // Create device control tabs
	createHelicalScanningControl();
	createFlimSystemControl();
    createAxsunOctSystemControl();

	// Set layout
	m_pVBoxLayout_DeviceOption->addStretch(1);
	m_pGroupBox_DeviceOption->setLayout(m_pVBoxLayout_DeviceOption);

	// Initialize
	if (parent_name == "Streaming")
	{
		m_pToggleButton_RotaryConnect->setChecked(true);
		m_pToggleButton_PullbackConnect->setChecked(true);
		m_pToggleButton_SynchronizedPulsedLaser->setChecked(true);
		m_pToggleButton_PmtGainVoltage->setChecked(true);
		m_pToggleButton_FlimLaserConnect->setChecked(true);
		m_pToggleButton_AxsunOctConnect->setChecked(true);
		m_pToggleButton_LightSource->setChecked(true);
#ifndef NEXT_GEN_SYSTEM
		m_pToggleButton_LiveImaging->setChecked(true);
#endif

		m_pToggleButton_RotaryConnect->setDisabled(true);
		m_pToggleButton_PullbackConnect->setDisabled(true);
		m_pToggleButton_SynchronizedPulsedLaser->setDisabled(true);
		m_pToggleButton_FlimLaserConnect->setDisabled(true);
		m_pToggleButton_AxsunOctConnect->setDisabled(true);
		m_pToggleButton_LightSource->setDisabled(true);
#ifndef NEXT_GEN_SYSTEM
		m_pToggleButton_LiveImaging->setDisabled(true);
#endif
	}
}

DeviceOptionTab::~DeviceOptionTab()
{
}


void DeviceOptionTab::createHelicalScanningControl()
{
	QGroupBox *pGroupBox_HelicalScanning = new QGroupBox;
	pGroupBox_HelicalScanning->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	pGroupBox_HelicalScanning->setFixedWidth(415);
	pGroupBox_HelicalScanning->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
	
	// Create widgets for Faulhaber motor control	
	m_pToggleButton_RotaryConnect = new QPushButton(this);
	m_pToggleButton_RotaryConnect->setText("Connect");
	m_pToggleButton_RotaryConnect->setFixedWidth(100);
	m_pToggleButton_RotaryConnect->setCheckable(true);
	m_pToggleButton_RotaryConnect->setStyleSheet("QPushButton { background-color:#ff0000; }");
	
	m_pLabel_RotaryConnect = new QLabel(this);
	m_pLabel_RotaryConnect->setText("Rotary Motor");

	m_pCheckBox_AutoApply = new QCheckBox(this);
	m_pCheckBox_AutoApply->setText("Auto ");
	//m_pCheckBox_AutoApply->setChecked(true);
	m_pCheckBox_AutoApply->setDisabled(true);

	m_pSpinBox_RPM = new QSpinBox(this);
	m_pSpinBox_RPM->setFixedWidth(50);
	m_pSpinBox_RPM->setRange(0, 6000);
	m_pSpinBox_RPM->setSingleStep(100);
	m_pSpinBox_RPM->setValue(m_pConfig->rotaryRpm); 
	m_pSpinBox_RPM->setAlignment(Qt::AlignCenter);
	m_pSpinBox_RPM->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pSpinBox_RPM->setDisabled(true);

	m_pLabel_RotationSpeed = new QLabel(this);
	m_pLabel_RotationSpeed->setText("Rotation Speed");
	m_pLabel_RotationSpeed->setDisabled(true);

	m_pLabel_RPM = new QLabel("RPM ", this);
	m_pLabel_RPM->setBuddy(m_pSpinBox_RPM);
	m_pLabel_RPM->setDisabled(true);

	m_pPushButton_RotateOperation = new QPushButton(this);
	m_pPushButton_RotateOperation->setText("Stop");
	m_pPushButton_RotateOperation->setFixedWidth(100);
	m_pPushButton_RotateOperation->setDisabled(true);

	m_pLabel_AutoVibCorrection = new QLabel(this); 
	m_pLabel_AutoVibCorrection->setText("Automatic Vib Correction");

	m_pToggleButton_AutoVibCorrection = new QPushButton(this);
	m_pToggleButton_AutoVibCorrection->setCheckable(true);
	m_pToggleButton_AutoVibCorrection->setText(!m_pConfig->autoVibCorrectionMode ? "Enable" : "Disable");
	m_pToggleButton_AutoVibCorrection->setFixedWidth(100);
	m_pToggleButton_AutoVibCorrection->setStyleSheet(!m_pConfig->autoVibCorrectionMode ? "QPushButton { background-color:#ff0000; }" : "QPushButton { background-color:#00ff00; }");
	m_pToggleButton_AutoVibCorrection->setChecked(m_pConfig->autoVibCorrectionMode);

	// Create widgets for pullback stage control
	m_pToggleButton_PullbackConnect = new QPushButton(this);
	m_pToggleButton_PullbackConnect->setText("Connect");
	m_pToggleButton_PullbackConnect->setFixedWidth(100);
	m_pToggleButton_PullbackConnect->setCheckable(true);
	m_pToggleButton_PullbackConnect->setStyleSheet("QPushButton { background-color:#ff0000; }");
	
	m_pLabel_PullbackConnect = new QLabel(this);
	m_pLabel_PullbackConnect->setText("Pullback Motor");

	m_pRadioButton_20mms50mm = new QRadioButton(this);
	m_pRadioButton_20mms50mm->setText("20, 50");
	m_pRadioButton_20mms50mm->setDisabled(true);
	m_pRadioButton_10mms30mm = new QRadioButton(this);
	m_pRadioButton_10mms30mm->setText("10, 30");
	m_pRadioButton_10mms30mm->setDisabled(true);
	m_pRadioButton_10mms40mm = new QRadioButton(this);
	m_pRadioButton_10mms40mm->setText("10, 40");
	m_pRadioButton_10mms40mm->setDisabled(true);
	m_pButtonGroup_PullbackMode = new QButtonGroup(this);
	m_pButtonGroup_PullbackMode->addButton(m_pRadioButton_20mms50mm, _20MM_S_50MM_);
	m_pButtonGroup_PullbackMode->addButton(m_pRadioButton_10mms30mm, _10MM_S_30MM_);
	m_pButtonGroup_PullbackMode->addButton(m_pRadioButton_10mms40mm, _10MM_S_40MM_);
	m_pLabel_PullbackMode = new QLabel("Pullback Mode (mm/s, mm)", this);
	m_pLabel_PullbackMode->setDisabled(true);

	if (m_pConfig->pullbackSpeed == 20.0)
		m_pRadioButton_20mms50mm->setChecked(true);
	else if (m_pConfig->pullbackSpeed == 10.0)
	{
		if (m_pConfig->pullbackLength == 30.0)
			m_pRadioButton_10mms30mm->setChecked(true);
		else if (m_pConfig->pullbackLength == 40.0)
			m_pRadioButton_10mms40mm->setChecked(true);
	}

	m_pPushButton_Pullback = new QPushButton(this);
	m_pPushButton_Pullback->setText("Pullback");
	m_pPushButton_Pullback->setFixedWidth(100);
	m_pPushButton_Pullback->setDisabled(true);
	m_pPushButton_Home = new QPushButton(this);
	m_pPushButton_Home->setText("Home");
	m_pPushButton_Home->setFixedWidth(49);
	m_pPushButton_Home->setDisabled(true);
	m_pPushButton_PullbackStop = new QPushButton(this);
	m_pPushButton_PullbackStop->setText("Stop");
	m_pPushButton_PullbackStop->setFixedWidth(48);
	m_pPushButton_PullbackStop->setDisabled(true);
	m_pLabel_PullbackOperation = new QLabel("Pullback Operation", this);
	m_pLabel_PullbackOperation->setDisabled(true);

	m_pLabel_PullbackFlagIndicator = new QLabel("", this);
	m_pLabel_PullbackFlagIndicator->setAlignment(Qt::AlignCenter);
	m_pLabel_PullbackFlagIndicator->setFixedWidth(39);
	m_pPushButton_PullbackFlagStateRenew = new QPushButton(this);
	m_pPushButton_PullbackFlagStateRenew->setText("Renew");
	m_pPushButton_PullbackFlagStateRenew->setFixedWidth(58);
	m_pPushButton_PullbackFlagStateRenew->setDisabled(true);
	m_pLabel_PullbackFlag = new QLabel("Pullback Flag", this);
	m_pLabel_PullbackFlag->setDisabled(true);

	m_pCheckBox_AutoHome = new QCheckBox(this);
	m_pCheckBox_AutoHome->setText("Auto Home");
	m_pCheckBox_AutoHome->setDisabled(true);
	m_pCheckBox_AutoHome->setChecked(m_pConfig->autoHomeMode);

	m_pLabel_AutoPullback = new QLabel(this);
	m_pLabel_AutoPullback->setText("Automatic Pullback");
	m_pLabel_AutoPullback->setDisabled(true);
	
	m_pLineEdit_AutoPullbackTime = new QLineEdit(this);
	m_pLineEdit_AutoPullbackTime->setFixedWidth(30);		
	m_pLineEdit_AutoPullbackTime->setText(QString::number(m_pConfig->autoPullbackTime));
	m_pLineEdit_AutoPullbackTime->setAlignment(Qt::AlignCenter);
	m_pLineEdit_AutoPullbackTime->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_AutoPullbackTime->setDisabled(true);
	
	m_pLabel_AutoPullbackSecond = new QLabel(this);
	m_pLabel_AutoPullbackSecond->setText("Sec ");
	m_pLabel_AutoPullbackSecond->setFixedWidth(25);
	m_pLabel_AutoPullbackSecond->setDisabled(true);

	m_pToggleButton_AutoPullback = new QPushButton(this);
	m_pToggleButton_AutoPullback->setCheckable(true);
	m_pToggleButton_AutoPullback->setText(!m_pConfig->autoPullbackMode ? "Enable" : "Disable");
	m_pToggleButton_AutoPullback->setChecked(m_pConfig->autoPullbackMode);
	m_pToggleButton_AutoPullback->setFixedWidth(100);
	m_pToggleButton_AutoPullback->setDisabled(true);
		
	// Set Layout
	QGridLayout *pGridLayout_RotaryMotor = new QGridLayout;
	pGridLayout_RotaryMotor->setSpacing(3);

	pGridLayout_RotaryMotor->addWidget(m_pLabel_RotaryConnect, 0, 0);
	pGridLayout_RotaryMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 1, 1, 4);
	pGridLayout_RotaryMotor->addWidget(m_pToggleButton_RotaryConnect, 0, 5);

	pGridLayout_RotaryMotor->addWidget(m_pLabel_RotationSpeed, 1, 0);
	pGridLayout_RotaryMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 1);
	pGridLayout_RotaryMotor->addWidget(m_pCheckBox_AutoApply, 1, 2);
	pGridLayout_RotaryMotor->addWidget(m_pSpinBox_RPM, 1, 3);
	pGridLayout_RotaryMotor->addWidget(m_pLabel_RPM, 1, 4);
	pGridLayout_RotaryMotor->addWidget(m_pPushButton_RotateOperation, 1, 5);

	pGridLayout_RotaryMotor->addWidget(m_pLabel_AutoVibCorrection, 2, 0);
	pGridLayout_RotaryMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 1, 1, 4);
	pGridLayout_RotaryMotor->addWidget(m_pToggleButton_AutoVibCorrection, 2, 5);

	QGridLayout *pGridLayout_PullbackMotor = new QGridLayout;
	pGridLayout_PullbackMotor->setSpacing(3);

	pGridLayout_PullbackMotor->addWidget(m_pLabel_PullbackConnect, 0, 0);
	pGridLayout_PullbackMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 1, 1, 3);
	pGridLayout_PullbackMotor->addWidget(m_pToggleButton_PullbackConnect, 0, 4, 1, 2, Qt::AlignRight);

	pGridLayout_PullbackMotor->addWidget(m_pLabel_PullbackMode, 1, 0);
	pGridLayout_PullbackMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 1);
	QHBoxLayout *pHBoxLayout_PullbackMode = new QHBoxLayout;
	pHBoxLayout_PullbackMode->setSpacing(3);
	pHBoxLayout_PullbackMode->addWidget(m_pRadioButton_20mms50mm);
	pHBoxLayout_PullbackMode->addWidget(m_pRadioButton_10mms30mm);
	pHBoxLayout_PullbackMode->addWidget(m_pRadioButton_10mms40mm);
	pGridLayout_PullbackMotor->addItem(pHBoxLayout_PullbackMode, 1, 2, 1, 4, Qt::AlignRight);

	pGridLayout_PullbackMotor->addWidget(m_pLabel_PullbackOperation, 2, 0);
	pGridLayout_PullbackMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 1, 1, 3);
	pGridLayout_PullbackMotor->addWidget(m_pPushButton_Pullback, 2, 4, 1, 2, Qt::AlignRight);

	pGridLayout_PullbackMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 3, 0, 1, 3);
	pGridLayout_PullbackMotor->addWidget(m_pCheckBox_AutoHome, 3, 3);
	pGridLayout_PullbackMotor->addWidget(m_pPushButton_Home, 3, 4, Qt::AlignRight);
	pGridLayout_PullbackMotor->addWidget(m_pPushButton_PullbackStop, 3, 5);

	//pGridLayout_PullbackMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 4, 0, 1, 4);
	//pGridLayout_PullbackMotor->addWidget(m_pCheckBox_AutoHome, 4, 4, 1, 2);

	pGridLayout_PullbackMotor->addWidget(m_pLabel_PullbackFlag, 5, 0);
	pGridLayout_PullbackMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 5, 1, 1, 3);
	QHBoxLayout *pHBoxLayout_PullbackFlag = new QHBoxLayout;
	pHBoxLayout_PullbackFlag->setSpacing(3);
	pHBoxLayout_PullbackFlag->addWidget(m_pLabel_PullbackFlagIndicator, Qt::AlignRight);
	pHBoxLayout_PullbackFlag->addWidget(m_pPushButton_PullbackFlagStateRenew);
	pGridLayout_PullbackMotor->addItem(pHBoxLayout_PullbackFlag, 5, 4, 1, 2, Qt::AlignRight);

	pGridLayout_PullbackMotor->addWidget(m_pLabel_AutoPullback, 6, 0);
	pGridLayout_PullbackMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 6, 1);
	QHBoxLayout *pHBoxLayout_AutoPullbackTime = new QHBoxLayout;
	pHBoxLayout_AutoPullbackTime->setSpacing(3);
	pHBoxLayout_AutoPullbackTime->addWidget(m_pLineEdit_AutoPullbackTime);
	pHBoxLayout_AutoPullbackTime->addWidget(m_pLabel_AutoPullbackSecond);
	pGridLayout_PullbackMotor->addItem(pHBoxLayout_AutoPullbackTime, 6, 2, 1, 2, Qt::AlignRight);	
	pGridLayout_PullbackMotor->addWidget(m_pToggleButton_AutoPullback, 6, 4, 1, 2, Qt::AlignRight);


	QVBoxLayout *pVBoxLayout_HelicalScanning = new QVBoxLayout;
	pVBoxLayout_HelicalScanning->setSpacing(10);
	
	pVBoxLayout_HelicalScanning->addItem(pGridLayout_RotaryMotor);
	pVBoxLayout_HelicalScanning->addItem(pGridLayout_PullbackMotor);

	pGroupBox_HelicalScanning->setLayout(pVBoxLayout_HelicalScanning);

	m_pVBoxLayout_DeviceOption->addWidget(pGroupBox_HelicalScanning);
	
	// Connect signal and slot
	connect(m_pToggleButton_RotaryConnect, SIGNAL(toggled(bool)), this, SLOT(connectRotaryMotor(bool)));
	connect(m_pCheckBox_AutoApply, SIGNAL(toggled(bool)), this, SLOT(changeAutoApply(bool)));
	connect(m_pToggleButton_AutoVibCorrection, SIGNAL(toggled(bool)), this, SLOT(enableAutoVibCorrectionMode(bool)));

	connect(m_pToggleButton_PullbackConnect, SIGNAL(toggled(bool)), this, SLOT(connectPullbackMotor(bool)));
	connect(m_pButtonGroup_PullbackMode, SIGNAL(buttonClicked(int)), this, SLOT(setPullbackMode(int)));
	connect(m_pPushButton_Pullback, SIGNAL(clicked(bool)), this, SLOT(moveAbsolute()));
	connect(m_pPushButton_Home, SIGNAL(clicked(bool)), this, SLOT(home()));
	connect(m_pPushButton_PullbackStop, SIGNAL(clicked(bool)), this, SLOT(stop()));
	connect(m_pPushButton_PullbackFlagStateRenew, SIGNAL(clicked(bool)), this, SLOT(renewPullbackFlag()));
	connect(m_pCheckBox_AutoHome, SIGNAL(toggled(bool)), this, SLOT(enableAutoHomeMode(bool)));
	connect(m_pLineEdit_AutoPullbackTime, SIGNAL(textChanged(const QString &)), this, SLOT(changeAutoPullbackTime(const QString &)));
	connect(m_pToggleButton_AutoPullback, SIGNAL(toggled(bool)), this, SLOT(enableAutoPullbackMode(bool)));
}

void DeviceOptionTab::createFlimSystemControl()
{
	QGroupBox *pGroupBox_FlimControl = new QGroupBox;
	pGroupBox_FlimControl->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	pGroupBox_FlimControl->setFixedWidth(415);
	pGroupBox_FlimControl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    // Create widgets for FLIM laser control		
	m_pToggleButton_AsynchronizedPulsedLaser = new QPushButton(this);
	m_pToggleButton_AsynchronizedPulsedLaser->setText("On");
	m_pToggleButton_AsynchronizedPulsedLaser->setFixedWidth(40);
	m_pToggleButton_AsynchronizedPulsedLaser->setCheckable(true);
	m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");
	///m_pToggleButton_AsynchronizedPulsedLaser->setDisabled(true);

	m_pLabel_AsynchronizedPulsedLaser = new QLabel("Asynchronized UV Pulsed Laser", this);
	m_pLabel_AsynchronizedPulsedLaser->setBuddy(m_pToggleButton_AsynchronizedPulsedLaser);
	///m_pLabel_AsynchronizedPulsedLaser->setDisabled(true);

	m_pToggleButton_SynchronizedPulsedLaser = new QPushButton(this);
	m_pToggleButton_SynchronizedPulsedLaser->setText("On");
	m_pToggleButton_SynchronizedPulsedLaser->setFixedWidth(40);
	m_pToggleButton_SynchronizedPulsedLaser->setCheckable(true);
	m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");
	///m_pToggleButton_SynchronizedPulsedLaser->setDisabled(true);

	m_pLabel_SynchronizedPulsedLaser = new QLabel("Synchronized UV Pulsed Laser", this);
	m_pLabel_SynchronizedPulsedLaser->setBuddy(m_pToggleButton_SynchronizedPulsedLaser);
	///m_pLabel_SynchronizedPulsedLaser->setDisabled(true);
	
	// Create widgets for PMT gain control
	m_pLineEdit_PmtGainVoltage = new QLineEdit(this);
	m_pLineEdit_PmtGainVoltage->setFixedWidth(45);
	m_pLineEdit_PmtGainVoltage->setText(QString::number(m_pConfig->pmtGainVoltage, 'f', 3));
	m_pLineEdit_PmtGainVoltage->setAlignment(Qt::AlignCenter);
	m_pLineEdit_PmtGainVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	///m_pLineEdit_PmtGainVoltage->setDisabled(true);

	m_pLabel_PmtGainControl = new QLabel("PMT Gain Voltage", this);
	m_pLabel_PmtGainControl->setBuddy(m_pLineEdit_PmtGainVoltage);
	///m_pLabel_PmtGainControl->setDisabled(true);

	m_pLabel_PmtGainVoltage = new QLabel("V  ", this);
	m_pLabel_PmtGainVoltage->setBuddy(m_pLineEdit_PmtGainVoltage);
	///m_pLabel_PmtGainVoltage->setDisabled(true);

	m_pToggleButton_PmtGainVoltage = new QPushButton(this);
	m_pToggleButton_PmtGainVoltage->setText("On");	
	m_pToggleButton_PmtGainVoltage->setFixedWidth(40);
	m_pToggleButton_PmtGainVoltage->setCheckable(true);
	m_pToggleButton_PmtGainVoltage->setStyleSheet("QPushButton { background-color:#ff0000; }");
	///m_pToggleButton_PmtGainVoltage->setDisabled(true);
	
	// Create widgets for FLIM laser power control
	m_pToggleButton_FlimLaserConnect = new QPushButton(this);
	m_pToggleButton_FlimLaserConnect->setText("Connect");
	m_pToggleButton_FlimLaserConnect->setFixedWidth(100);
	m_pToggleButton_FlimLaserConnect->setCheckable(true);
	m_pToggleButton_FlimLaserConnect->setStyleSheet("QPushButton { background-color:#ff0000; }");
	
	m_pLabel_FlimLaserConnect = new QLabel(this);
	m_pLabel_FlimLaserConnect->setText("FLIm UV Pulsed Laser");

#ifndef NEXT_GEN_SYSTEM
	m_pLineEdit_FlimLaserPowerMonitor = new QLineEdit(this);
	m_pLineEdit_FlimLaserPowerMonitor->setReadOnly(true);
	m_pLineEdit_FlimLaserPowerMonitor->setFixedWidth(30);
	m_pLineEdit_FlimLaserPowerMonitor->setAlignment(Qt::AlignCenter);
	m_pLineEdit_FlimLaserPowerMonitor->setDisabled(true);

	m_pPushButton_FlimLaserPowerIncrease = new QPushButton(this);
	m_pPushButton_FlimLaserPowerIncrease->setText(QString::fromLocal8Bit("¡ã"));
	m_pPushButton_FlimLaserPowerIncrease->setFixedWidth(20);
	m_pPushButton_FlimLaserPowerIncrease->setDisabled(true);

	m_pPushButton_FlimLaserPowerDecrease = new QPushButton(this);
	m_pPushButton_FlimLaserPowerDecrease->setText(QString::fromLocal8Bit("¡å"));
	m_pPushButton_FlimLaserPowerDecrease->setFixedWidth(20);
	m_pPushButton_FlimLaserPowerDecrease->setDisabled(true);
#else
	m_pLineEdit_FlimLaserPowerControl = new QLineEdit(this);
	m_pLineEdit_FlimLaserPowerControl->setValidator(new QIntValidator(0, 255, this));
	m_pLineEdit_FlimLaserPowerControl->setText(QString::number(m_pConfig->flimLaserPower));
	m_pLineEdit_FlimLaserPowerControl->setAlignment(Qt::AlignCenter);
	m_pLineEdit_FlimLaserPowerControl->setFixedWidth(40);
	m_pLineEdit_FlimLaserPowerControl->setDisabled(true);

	m_pPushButton_FlimLaserPowerControl = new QPushButton(this);
	m_pPushButton_FlimLaserPowerControl->setText("Apply");
	m_pPushButton_FlimLaserPowerControl->setFixedWidth(50);
	m_pPushButton_FlimLaserPowerControl->setDisabled(true);
#endif

	m_pLabel_FlimLaserPowerControl = new QLabel("Laser Power Level Adjustment", this);
	m_pLabel_FlimLaserPowerControl->setFixedWidth(205);	
	m_pLabel_FlimLaserPowerControl->setDisabled(true);

	// Set Layout	
	QGridLayout *pGridLayout_Synchornization = new QGridLayout;
	pGridLayout_Synchornization->setSpacing(3);

	pGridLayout_Synchornization->addWidget(m_pLabel_AsynchronizedPulsedLaser, 0, 0);
	pGridLayout_Synchornization->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 1);
	pGridLayout_Synchornization->addWidget(m_pToggleButton_AsynchronizedPulsedLaser, 0, 2);

	pGridLayout_Synchornization->addWidget(m_pLabel_SynchronizedPulsedLaser, 1, 0);
	pGridLayout_Synchornization->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 1);
	pGridLayout_Synchornization->addWidget(m_pToggleButton_SynchronizedPulsedLaser, 1, 2);

	QGridLayout *pGridLayout_PmtGainControl = new QGridLayout;
	pGridLayout_PmtGainControl->setSpacing(3);
	
	pGridLayout_PmtGainControl->addWidget(m_pLabel_PmtGainControl, 0, 0);
	pGridLayout_PmtGainControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 1);
	pGridLayout_PmtGainControl->addWidget(m_pLineEdit_PmtGainVoltage, 0, 2);
	pGridLayout_PmtGainControl->addWidget(m_pLabel_PmtGainVoltage, 0, 3);
	pGridLayout_PmtGainControl->addWidget(m_pToggleButton_PmtGainVoltage, 0, 4);
	
	QGridLayout *pGridLayout_FlimLaser = new QGridLayout;
	pGridLayout_FlimLaser->setSpacing(3);

	pGridLayout_FlimLaser->addWidget(m_pLabel_FlimLaserConnect, 0, 0);
	pGridLayout_FlimLaser->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 1, 1, 2);	
	pGridLayout_FlimLaser->addWidget(m_pToggleButton_FlimLaserConnect, 0, 3, 1, 4, Qt::AlignRight); 

	pGridLayout_FlimLaser->addWidget(m_pLabel_FlimLaserPowerControl, 1, 0);
	pGridLayout_FlimLaser->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 1, 1, 3); 
#ifndef NEXT_GEN_SYSTEM
	pGridLayout_FlimLaser->addWidget(m_pLineEdit_FlimLaserPowerMonitor, 1, 4);
	pGridLayout_FlimLaser->addWidget(m_pPushButton_FlimLaserPowerIncrease, 1, 5, Qt::AlignRight);
	pGridLayout_FlimLaser->addWidget(m_pPushButton_FlimLaserPowerDecrease, 1, 6, Qt::AlignRight);
#else
	pGridLayout_FlimLaser->addWidget(m_pLineEdit_FlimLaserPowerControl, 1, 3, Qt::AlignRight);
	pGridLayout_FlimLaser->addWidget(m_pPushButton_FlimLaserPowerControl, 1, 4, Qt::AlignRight);
#endif

	QVBoxLayout *pVBoxLayout_FlimControl = new QVBoxLayout;
	pVBoxLayout_FlimControl->setSpacing(10);

	pVBoxLayout_FlimControl->addItem(pGridLayout_Synchornization);
	pVBoxLayout_FlimControl->addItem(pGridLayout_PmtGainControl);
	pVBoxLayout_FlimControl->addItem(pGridLayout_FlimLaser);

	pGroupBox_FlimControl->setLayout(pVBoxLayout_FlimControl);

	m_pVBoxLayout_DeviceOption->addWidget(pGroupBox_FlimControl);
	
    // Connect signal and slot		
	connect(m_pToggleButton_AsynchronizedPulsedLaser, SIGNAL(toggled(bool)), this, SLOT(startFlimAsynchronization(bool)));
	connect(m_pToggleButton_SynchronizedPulsedLaser, SIGNAL(toggled(bool)), this, SLOT(startFlimSynchronization(bool)));
		
	connect(m_pLineEdit_PmtGainVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changePmtGainVoltage(const QString &)));
    connect(m_pToggleButton_PmtGainVoltage, SIGNAL(toggled(bool)), this, SLOT(applyPmtGainVoltage(bool)));

	connect(m_pToggleButton_FlimLaserConnect, SIGNAL(toggled(bool)), this, SLOT(connectFlimLaser(bool)));	
#ifndef NEXT_GEN_SYSTEM
	connect(m_pPushButton_FlimLaserPowerIncrease, SIGNAL(clicked(bool)), this, SLOT(increaseLaserPower()));
	connect(m_pPushButton_FlimLaserPowerDecrease, SIGNAL(clicked(bool)), this, SLOT(decreaseLaserPower()));
	connect(this, &DeviceOptionTab::showCurrentUVPower, [&](int level) { m_pLineEdit_FlimLaserPowerMonitor->setText(QString::number(level)); }); // crash occurred
#else
	connect(m_pLineEdit_FlimLaserPowerControl, SIGNAL(textChanged(const QString &)), this, SLOT(changeFlimLaserPower(const QString &)));
	connect(m_pPushButton_FlimLaserPowerControl, SIGNAL(clicked(bool)), this, SLOT(applyFlimLaserPower()));
#endif
}

void DeviceOptionTab::createAxsunOctSystemControl()
{
	QGroupBox *pGroupBox_OctControl = new QGroupBox;
	pGroupBox_OctControl->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	pGroupBox_OctControl->setFixedWidth(415);
	pGroupBox_OctControl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

	// Create widgets for Axsun OCT control
	m_pToggleButton_AxsunOctConnect = new QPushButton(this);
	m_pToggleButton_AxsunOctConnect->setText("Connect");
	m_pToggleButton_AxsunOctConnect->setFixedWidth(100);
	m_pToggleButton_AxsunOctConnect->setCheckable(true);
	m_pToggleButton_AxsunOctConnect->setStyleSheet("QPushButton { background-color:#ff0000; }");
	
	m_pLabel_AxsunOctConnect = new QLabel(this);
	m_pLabel_AxsunOctConnect->setText("Axsun OCT Engine");
	
    m_pToggleButton_LightSource = new QPushButton(this);
    m_pToggleButton_LightSource->setCheckable(true);
    m_pToggleButton_LightSource->setFixedWidth(48);
    m_pToggleButton_LightSource->setText("On");
    m_pToggleButton_LightSource->setDisabled(true);

	m_pLabel_LightSource = new QLabel("Swept-Source OCT Laser", this);
	m_pLabel_LightSource->setBuddy(m_pToggleButton_LightSource);
	m_pLabel_LightSource->setDisabled(true);

#ifndef NEXT_GEN_SYSTEM
	m_pToggleButton_LiveImaging = new QPushButton(this);
	m_pToggleButton_LiveImaging->setCheckable(true);
	m_pToggleButton_LiveImaging->setFixedWidth(48);
	m_pToggleButton_LiveImaging->setText("On");
	m_pToggleButton_LiveImaging->setDisabled(true);

	m_pLabel_LiveImaging = new QLabel("Axsun OCT Live Imaging Mode", this);
	m_pLabel_LiveImaging->setBuddy(m_pToggleButton_LiveImaging);
	m_pLabel_LiveImaging->setDisabled(true);
#endif

	m_pLabel_PipelineMode = new QLabel("Pipeline Mode", this);	
	m_pLabel_PipelineMode->setDisabled(true);
	
	m_pRadioButton_JpegCompressed = new QRadioButton(this);
	m_pRadioButton_JpegCompressed->setText("JPEG ");	
	m_pRadioButton_JpegCompressed->setDisabled(true);	

	m_pRadioButton_RawAdcData = new QRadioButton(this);
	m_pRadioButton_RawAdcData->setText("Raw ADC Data");
	m_pRadioButton_RawAdcData->setDisabled(true);

	if (m_pConfig->axsunPipelineMode == 0)
		m_pRadioButton_JpegCompressed->setChecked(true);
	else
		m_pRadioButton_RawAdcData->setChecked(true);
	
	m_pButtonGroup_PipelineMode = new QButtonGroup(this);
	m_pButtonGroup_PipelineMode->addButton(m_pRadioButton_JpegCompressed, JPEG_COMPRESSED);
	m_pButtonGroup_PipelineMode->addButton(m_pRadioButton_RawAdcData, RAW_ADC_DATA);
	
	m_pLabel_BackgroundSubtraction = new QLabel("Background Subtraction", this);
	m_pLabel_BackgroundSubtraction->setDisabled(true);

	m_pLabel_DispersionCompensation = new QLabel("Dispersion Compensation (a2, a3)", this);
	m_pLabel_DispersionCompensation->setDisabled(true);

	m_pPushButton_BgSet = new QPushButton(this);
	m_pPushButton_BgSet->setText("Set");
	m_pPushButton_BgSet->setFixedWidth(50);
	m_pPushButton_BgSet->setDisabled(true);

	m_pPushButton_BgReset = new QPushButton(this);
	m_pPushButton_BgReset->setText("Reset");
	m_pPushButton_BgReset->setFixedWidth(50);
	m_pPushButton_BgReset->setDisabled(true);
	
	m_pLineEdit_DispComp_a2 = new QLineEdit(this);
	m_pLineEdit_DispComp_a2->setFixedWidth(30);
	m_pLineEdit_DispComp_a2->setText(QString::number(m_pConfig->axsunDispComp_a2, 'f', 1));
	m_pLineEdit_DispComp_a2->setAlignment(Qt::AlignCenter);
	m_pLineEdit_DispComp_a2->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_DispComp_a2->setDisabled(true);

	m_pLineEdit_DispComp_a3 = new QLineEdit(this);
	m_pLineEdit_DispComp_a3->setFixedWidth(30);
	m_pLineEdit_DispComp_a3->setText(QString::number(m_pConfig->axsunDispComp_a3, 'f', 1));
	m_pLineEdit_DispComp_a3->setAlignment(Qt::AlignCenter);
	m_pLineEdit_DispComp_a3->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_DispComp_a3->setDisabled(true);

	m_pPushButton_Compensate = new QPushButton(this);
	m_pPushButton_Compensate->setText("Apply");
	m_pPushButton_Compensate->setFixedWidth(50);
	m_pPushButton_Compensate->setDisabled(true);
		
    m_pSpinBox_VDLLength = new QMySpinBox(this);
    m_pSpinBox_VDLLength->setFixedWidth(55);
    m_pSpinBox_VDLLength->setRange(0.00, 15.00);
    m_pSpinBox_VDLLength->setSingleStep(0.05);
    m_pSpinBox_VDLLength->setValue(m_pConfig->axsunVDLLength);
    m_pSpinBox_VDLLength->setDecimals(2);
    m_pSpinBox_VDLLength->setAlignment(Qt::AlignCenter);
    m_pSpinBox_VDLLength->setDisabled(true);

    m_pLabel_VDLLength = new QLabel("VDL Position Adjustment (mm)", this);
    m_pLabel_VDLLength->setBuddy(m_pSpinBox_VDLLength);
    m_pLabel_VDLLength->setDisabled(true);

	m_pPushButton_VDLHome = new QPushButton(this);
	m_pPushButton_VDLHome->setFixedWidth(48);
	m_pPushButton_VDLHome->setText("Home");
	m_pPushButton_VDLHome->setDisabled(true);
	
	// Set Layout
	QGridLayout *pGridLayout_AxsunControl = new QGridLayout;
	pGridLayout_AxsunControl->setSpacing(3);

	pGridLayout_AxsunControl->addWidget(m_pLabel_AxsunOctConnect, 0, 0);
	pGridLayout_AxsunControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 1);
	pGridLayout_AxsunControl->addWidget(m_pToggleButton_AxsunOctConnect, 0, 2, 1, 2, Qt::AlignRight);

	pGridLayout_AxsunControl->addWidget(m_pLabel_LightSource, 1, 0);
	pGridLayout_AxsunControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 1, 1, 2);
	pGridLayout_AxsunControl->addWidget(m_pToggleButton_LightSource, 1, 3);

#ifndef NEXT_GEN_SYSTEM
	pGridLayout_AxsunControl->addWidget(m_pLabel_LiveImaging, 2, 0);
	pGridLayout_AxsunControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 1, 1, 2);
	pGridLayout_AxsunControl->addWidget(m_pToggleButton_LiveImaging, 2, 3);
#endif

	QHBoxLayout *pHBoxLayout_Pipeline = new QHBoxLayout;
	pHBoxLayout_Pipeline->setSpacing(3);

	pHBoxLayout_Pipeline->addWidget(m_pLabel_PipelineMode);
	pHBoxLayout_Pipeline->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
	pHBoxLayout_Pipeline->addWidget(m_pRadioButton_JpegCompressed);
	pHBoxLayout_Pipeline->addWidget(m_pRadioButton_RawAdcData);
	
	QHBoxLayout *pHBoxLayout_BgSub = new QHBoxLayout;
	pHBoxLayout_BgSub->setSpacing(3);

	pHBoxLayout_BgSub->addWidget(m_pLabel_BackgroundSubtraction);
	pHBoxLayout_BgSub->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
	pHBoxLayout_BgSub->addWidget(m_pPushButton_BgSet);
	pHBoxLayout_BgSub->addWidget(m_pPushButton_BgReset);

	QHBoxLayout *pHBoxLayout_DispComp = new QHBoxLayout;
	pHBoxLayout_DispComp->setSpacing(3);

	pHBoxLayout_DispComp->addWidget(m_pLabel_DispersionCompensation);
	pHBoxLayout_DispComp->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
	pHBoxLayout_DispComp->addWidget(m_pLineEdit_DispComp_a2);
	pHBoxLayout_DispComp->addWidget(m_pLineEdit_DispComp_a3);
	pHBoxLayout_DispComp->addWidget(m_pPushButton_Compensate);
	
	pGridLayout_AxsunControl->addItem(pHBoxLayout_Pipeline, 3, 0, 1, 4);
	pGridLayout_AxsunControl->addItem(pHBoxLayout_BgSub, 4, 0, 1, 4);
	pGridLayout_AxsunControl->addItem(pHBoxLayout_DispComp, 5, 0, 1, 4);

	pGridLayout_AxsunControl->addWidget(m_pLabel_VDLLength, 6, 0);
	pGridLayout_AxsunControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 6, 1);
	pGridLayout_AxsunControl->addWidget(m_pSpinBox_VDLLength, 6, 2);
	pGridLayout_AxsunControl->addWidget(m_pPushButton_VDLHome, 6, 3);


	QVBoxLayout *pVBoxLayout_OctControl = new QVBoxLayout;
	pVBoxLayout_OctControl->setSpacing(10);

	pVBoxLayout_OctControl->addItem(pGridLayout_AxsunControl);

	pGroupBox_OctControl->setLayout(pVBoxLayout_OctControl);

	m_pVBoxLayout_DeviceOption->addWidget(pGroupBox_OctControl);
	
    // Connect signal and slot
	connect(m_pToggleButton_AxsunOctConnect, SIGNAL(toggled(bool)), this, SLOT(connectAxsunControl(bool)));
	connect(m_pToggleButton_LightSource, SIGNAL(toggled(bool)), this, SLOT(setLightSource(bool)));
#ifndef NEXT_GEN_SYSTEM
	connect(m_pToggleButton_LiveImaging, SIGNAL(toggled(bool)), this, SLOT(setLiveImaging(bool)));
#endif
	connect(m_pButtonGroup_PipelineMode, SIGNAL(buttonClicked(int)), this, SLOT(setPipelineMode(int)));
	connect(m_pPushButton_BgSet, SIGNAL(clicked(bool)), this, SLOT(setBackground()));
	connect(m_pPushButton_BgReset, SIGNAL(clicked(bool)), this, SLOT(resetBackground()));
	connect(m_pPushButton_Compensate, SIGNAL(clicked(bool)), this, SLOT(setDispersionCompensation()));
	connect(m_pSpinBox_VDLLength, SIGNAL(valueChanged(double)), this, SLOT(setVDLLength(double)));
	connect(m_pPushButton_VDLHome, SIGNAL(clicked(bool)), this, SLOT(setVDLHome()));
}


bool DeviceOptionTab::connectRotaryMotor(bool toggled)
{
	if (toggled)
	{
		// Connect to rotary motor
		if (m_pDeviceControl->connectRotaryMotor(true))
		{
			// Set widgets
			m_pToggleButton_RotaryConnect->setText("Disconnect");
			m_pToggleButton_RotaryConnect->setStyleSheet("QPushButton { background-color:#00ff00; }");

			m_pLabel_RotaryConnect->setDisabled(true);

			m_pCheckBox_AutoApply->setEnabled(true);
			m_pLabel_RotationSpeed->setEnabled(true);
			m_pSpinBox_RPM->setEnabled(true);
			m_pLabel_RPM->setEnabled(true);
			m_pPushButton_RotateOperation->setEnabled(true);

			changeAutoApply(m_pCheckBox_AutoApply->isChecked());
		}
		else
			m_pToggleButton_RotaryConnect->setChecked(false);
	}
	else
	{
		// Stop first
		rotateStop();

		// Set widgets
		m_pToggleButton_RotaryConnect->setText("Connect");
		m_pToggleButton_RotaryConnect->setStyleSheet("QPushButton { background-color:#ff0000; }");

		m_pLabel_RotaryConnect->setEnabled(true);

		m_pCheckBox_AutoApply->setDisabled(true);
		m_pLabel_RotationSpeed->setDisabled(true);
		m_pSpinBox_RPM->setDisabled(true);
		m_pLabel_RPM->setDisabled(true);
		m_pPushButton_RotateOperation->setDisabled(true);

		// Disconnect from rotary motor
		m_pDeviceControl->connectRotaryMotor(false);
	}

	return true;
}

void DeviceOptionTab::changeAutoApply(bool toggled)
{
	disconnect(m_pSpinBox_RPM, SIGNAL(valueChanged(int)), 0, 0);
	disconnect(m_pPushButton_RotateOperation, SIGNAL(clicked(bool)), 0, 0);

	if (toggled)
	{
		m_pPushButton_RotateOperation->setText("Stop");
		connect(m_pSpinBox_RPM, SIGNAL(valueChanged(int)), this, SLOT(changeRotaryRpm(int)));
		connect(m_pPushButton_RotateOperation, SIGNAL(clicked(bool)), this, SLOT(rotateStop()));
	}
	else
	{
		m_pPushButton_RotateOperation->setText("Rotate");
		connect(m_pPushButton_RotateOperation, SIGNAL(clicked(bool)), this, SLOT(rotate()));
	}
}

void DeviceOptionTab::changeRotaryRpm(int rpm)
{
	m_pConfig->rotaryRpm = rpm;	
	m_pDeviceControl->changeRotaryRpm(rpm);
	
	m_pConfig->writeToLog(QString("Rotation RPM set: %1 rpm").arg(m_pConfig->rotaryRpm));
}

void DeviceOptionTab::rotateStop()
{
	m_pDeviceControl->rotateStop();
	m_pSpinBox_RPM->setValue(0);

	if (m_pStreamTab)
		m_pStreamTab->enableRotation(false);

	m_pConfig->writeToLog("Rotation stop");
}

void DeviceOptionTab::rotate()
{
	int rpm = m_pSpinBox_RPM->value();

	if (rpm != 0)
		changeRotaryRpm(rpm);
	else
		rotateStop();
}

void DeviceOptionTab::enableAutoVibCorrectionMode(bool toggled)
{
	m_pConfig->autoVibCorrectionMode = toggled;
	if (toggled)
	{
		m_pToggleButton_AutoVibCorrection->setText("Disable");
		m_pToggleButton_AutoVibCorrection->setStyleSheet("QPushButton { background-color:#00ff00; }");
	}
	else
	{
		m_pToggleButton_AutoVibCorrection->setText("Enable");
		m_pToggleButton_AutoVibCorrection->setStyleSheet("QPushButton { background-color:#ff0000; }");
	}
}


bool DeviceOptionTab::connectPullbackMotor(bool toggled)
{
	if (toggled)
	{
		// Connect to pullback motor
		if (m_pDeviceControl->connectPullbackMotor(true))
		{
			// Set widgets
			m_pToggleButton_PullbackConnect->setText("Disconnect");
			m_pToggleButton_PullbackConnect->setStyleSheet("QPushButton { background-color:#00ff00; }");

			m_pLabel_PullbackConnect->setDisabled(true);

			m_pLabel_PullbackMode->setEnabled(true);
			m_pRadioButton_20mms50mm->setEnabled(true);
			m_pRadioButton_10mms30mm->setEnabled(true);
			m_pRadioButton_10mms40mm->setEnabled(true);

			m_pLabel_PullbackOperation->setEnabled(true);
			m_pPushButton_Pullback->setEnabled(true);
			m_pPushButton_Home->setEnabled(true);
			m_pPushButton_PullbackStop->setEnabled(true);
			m_pPushButton_Pullback->setStyleSheet("QPushButton { background-color:#ff0000; }");
			m_pLabel_PullbackFlag->setEnabled(true);
			m_pPushButton_PullbackFlagStateRenew->setEnabled(true);			
			m_pLabel_PullbackFlagIndicator->setText(!m_pConfig->pullbackFlag ? "On" : "Off");
			m_pLabel_PullbackFlagIndicator->setStyleSheet(!m_pConfig->pullbackFlag ? "QLabel { background-color:#00ff00; color:black; }" : "QLabel { background-color:#ff0000; color:white; }");
			m_pCheckBox_AutoHome->setEnabled(true);
			m_pLabel_AutoPullback->setEnabled(true);
			if (m_pConfig->autoPullbackMode)
			{
				m_pLineEdit_AutoPullbackTime->setEnabled(true);
				m_pLabel_AutoPullbackSecond->setEnabled(true);
			}
			m_pToggleButton_AutoPullback->setEnabled(true);
			m_pToggleButton_AutoPullback->setStyleSheet(!m_pConfig->autoPullbackMode ? "QPushButton { background-color:#ff0000; }" : "QPushButton { background-color:#00ff00; }");

			connect(this, SIGNAL(stopPullback(bool)), this, SLOT(setPullbackWidgets(bool)));
		}
		else
			m_pToggleButton_PullbackConnect->setChecked(false);
	}
	else
	{
		// Set widgets
		m_pToggleButton_PullbackConnect->setText("Connect");
		m_pToggleButton_PullbackConnect->setStyleSheet("QPushButton { background-color:#ff0000; }");

		m_pLabel_PullbackConnect->setEnabled(true);

		m_pLabel_PullbackMode->setDisabled(true);
		m_pRadioButton_20mms50mm->setDisabled(true);
		m_pRadioButton_10mms30mm->setDisabled(true);
		m_pRadioButton_10mms40mm->setDisabled(true);

		m_pLabel_PullbackOperation->setDisabled(true);
		m_pPushButton_Pullback->setDisabled(true);
		m_pPushButton_Home->setDisabled(true);
		m_pPushButton_PullbackStop->setDisabled(true);
		m_pPushButton_Pullback->setStyleSheet("QPushButton { background-color:#353535; }");
		m_pLabel_PullbackFlag->setDisabled(true);
		m_pPushButton_PullbackFlagStateRenew->setDisabled(true);
		m_pLabel_PullbackFlagIndicator->setText("");
		m_pLabel_PullbackFlagIndicator->setStyleSheet("QLabel { background-color:#353535; }");
		m_pCheckBox_AutoHome->setDisabled(true);
		m_pLabel_AutoPullback->setDisabled(true);
		m_pLineEdit_AutoPullbackTime->setDisabled(true);
		m_pLabel_AutoPullbackSecond->setDisabled(true);
		m_pToggleButton_AutoPullback->setDisabled(true);
		m_pToggleButton_AutoPullback->setStyleSheet("QPushButton { background-color:#353535; }");

		// Disconnect from rotary motor
		m_pDeviceControl->connectRotaryMotor(false);
	}
		
	return true;
}

void DeviceOptionTab::setPullbackMode(int id)
{
	if (id == _20MM_S_50MM_)
	{
		m_pDeviceControl->setTargetSpeed(20.0);
		m_pDeviceControl->changePullbackLength(50.0);
	}
	else if (id == _10MM_S_30MM_)
	{
		m_pDeviceControl->setTargetSpeed(10.0);
		m_pDeviceControl->changePullbackLength(30.0);
	}
	else if (id == _10MM_S_40MM_)
	{
		m_pDeviceControl->setTargetSpeed(10.0);
		m_pDeviceControl->changePullbackLength(40.0);
	}
}

void DeviceOptionTab::moveAbsolute()
{
	m_pDeviceControl->getPullbackMotor()->DidRotateEnd.clear();
	m_pDeviceControl->getPullbackMotor()->DidRotateEnd += [&](int) 
	{
		// Set widgets after pullback finished
		emit stopPullback(true);
	};

	// Pullback
	if (!m_pConfig->pullbackFlag)
	{
		m_pConfig->pullbackFlag = true;
		m_pConfig->setConfigFile("Havana3.ini");
		m_pDeviceControl->moveAbsolute();
		renewPullbackFlag();
		
		// Set widgets
		setPullbackWidgets(false);
	}
	else
		stopPullback(true);
}

void DeviceOptionTab::home()
{
	m_pDeviceControl->getPullbackMotor()->DidRotateEnd.clear();
	m_pDeviceControl->getPullbackMotor()->DidRotateEnd += [&](int timeout)
	{
		if (timeout)
		{
			m_pConfig->pullbackFlag = false;
			m_pConfig->setConfigFile("Havana3.ini");
			renewPullbackFlag();
		}
	};
	m_pDeviceControl->home();
}

void DeviceOptionTab::stop()
{
	m_pDeviceControl->stop();
	setPullbackWidgets(true);
}

void DeviceOptionTab::renewPullbackFlag()
{
	m_pLabel_PullbackFlagIndicator->setText(!m_pConfig->pullbackFlag ? "On" : "Off");
	m_pLabel_PullbackFlagIndicator->setStyleSheet(!m_pConfig->pullbackFlag ? "QLabel { background-color:#00ff00; color:black; }" : "QLabel { background-color:#ff0000; color:white; }");
}

void DeviceOptionTab::enableAutoHomeMode(bool enabled)
{
	m_pConfig->autoHomeMode = enabled;
}

void DeviceOptionTab::setPullbackWidgets(bool enabled)
{
	// Set widgets
	m_pPushButton_Home->setEnabled(enabled);
	if (enabled)
		m_pPushButton_Pullback->setStyleSheet("QPushButton { background-color:#ff0000; }");
	else
		m_pPushButton_Pullback->setStyleSheet("QPushButton { background-color:#00ff00; }");
}

void DeviceOptionTab::changeAutoPullbackTime(const QString &str)
{
	m_pConfig->autoPullbackTime = str.toInt();

	m_pConfig->writeToLog(QString("Auto pullback time set: %1 sec").arg(m_pConfig->autoPullbackTime));
}

void DeviceOptionTab::enableAutoPullbackMode(bool toggled)
{
	m_pConfig->autoPullbackMode = toggled;
	if (toggled)
	{
		m_pToggleButton_AutoPullback->setText("Disable");
		m_pToggleButton_AutoPullback->setStyleSheet("QPushButton { background-color:#00ff00; }");
		m_pLineEdit_AutoPullbackTime->setEnabled(true);
		m_pLabel_AutoPullbackSecond->setEnabled(true);
	}
	else
	{
		m_pToggleButton_AutoPullback->setText("Enable");
		m_pToggleButton_AutoPullback->setStyleSheet("QPushButton { background-color:#ff0000; }");
		m_pLineEdit_AutoPullbackTime->setDisabled(true);
		m_pLabel_AutoPullbackSecond->setDisabled(true);
	}
}


void DeviceOptionTab::startFlimAsynchronization(bool toggled)
{
	if (toggled)
	{
#ifdef NI_ENABLE
		// Start asynchronous pulsed generation
		if (m_pDeviceControl->startSynchronization(true, true))
		{
			// Set widgets		
			m_pToggleButton_AsynchronizedPulsedLaser->setText("Off");
			m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#00ff00; }");

			m_pLabel_SynchronizedPulsedLaser->setDisabled(true);
			m_pToggleButton_SynchronizedPulsedLaser->setDisabled(true);
			m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#353535; }");
		}
		else
			m_pToggleButton_AsynchronizedPulsedLaser->setChecked(false);
#else
		m_pToggleButton_AsynchronizedPulsedLaser->setChecked(false);
#endif
	}
	else
	{
		// Set widgets
		m_pToggleButton_AsynchronizedPulsedLaser->setText("On");
		m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");
				
		m_pLabel_SynchronizedPulsedLaser->setEnabled(true);
		m_pToggleButton_SynchronizedPulsedLaser->setEnabled(true);
		m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");

		// Stop asynchronous pulsed generation
		m_pDeviceControl->startSynchronization(false);
	}
}

void DeviceOptionTab::startFlimSynchronization(bool toggled)
{
	if (toggled)
	{
#ifdef NI_ENABLE
		// Start synchronous OCT-FLIm operation
		if (m_pDeviceControl->startSynchronization(true))
		{
			// Set widgets		
			m_pToggleButton_SynchronizedPulsedLaser->setText("Off");
			m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#00ff00; }");

			m_pLabel_AsynchronizedPulsedLaser->setDisabled(true);
			m_pToggleButton_AsynchronizedPulsedLaser->setDisabled(true);
			m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#353535; }");
		}
		else
			m_pToggleButton_SynchronizedPulsedLaser->setChecked(false);
#else
		m_pToggleButton_SynchronizedPulsedLaser->setChecked(false);
#endif
	}
	else
	{
		// Set widgets
		m_pToggleButton_SynchronizedPulsedLaser->setText("On");
		m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");

		m_pLabel_AsynchronizedPulsedLaser->setEnabled(true);
		m_pToggleButton_AsynchronizedPulsedLaser->setEnabled(true);
		m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");

		// Stop synchronous OCT-FLIm operation
		m_pDeviceControl->startSynchronization(false);
	}
}


void DeviceOptionTab::applyPmtGainVoltage(bool toggled)
{
	if (toggled)
	{
#ifdef NI_ENABLE	
		// Apply PMT gain voltage
		if (m_pDeviceControl->applyPmtGainVoltage(true)) 
		{
			// Set widgets
			m_pToggleButton_PmtGainVoltage->setText("Off");
			m_pToggleButton_PmtGainVoltage->setStyleSheet("QPushButton { background-color:#00ff00; }");

			m_pLineEdit_PmtGainVoltage->setDisabled(true);
			m_pLabel_PmtGainVoltage->setDisabled(true);
		}
		else
			m_pToggleButton_PmtGainVoltage->setChecked(false);
#else
		m_pToggleButton_PmtGainVoltage->setChecked(false);
#endif
	}
	else
	{
		// Set widgets
		m_pToggleButton_PmtGainVoltage->setText("On");
		m_pToggleButton_PmtGainVoltage->setStyleSheet("QPushButton { background-color:#ff0000; }");

		m_pLineEdit_PmtGainVoltage->setEnabled(true);
		m_pLabel_PmtGainVoltage->setEnabled(true);

		// Stop applying PMT gain voltage
		m_pDeviceControl->applyPmtGainVoltage(false);
	}	
}

void DeviceOptionTab::changePmtGainVoltage(const QString & str)
{
	m_pConfig->pmtGainVoltage = str.toFloat();

	m_pConfig->writeToLog(QString("PMT gain voltage set: %1 mm").arg(m_pConfig->pmtGainVoltage, 3, 'f', 2));
}


bool DeviceOptionTab::connectFlimLaser(bool toggled)
{
	if (toggled)
	{
		// Connect to FLIm laser
		if (m_pDeviceControl->connectFlimLaser(true))
		{
			// Set callback
			m_pDeviceControl->getElforlightLaser()->UpdatePowerLevel += [&](int laser_power_level) {
				emit showCurrentUVPower(laser_power_level);
			};

			// Set widgets
			m_pToggleButton_FlimLaserConnect->setText("Disconnect");
			m_pToggleButton_FlimLaserConnect->setStyleSheet("QPushButton { background-color:#00ff00; }");

			m_pLabel_FlimLaserConnect->setDisabled(true);

			m_pLabel_FlimLaserPowerControl->setEnabled(true);
#ifndef NEXT_GEN_SYSTEM
			m_pLineEdit_FlimLaserPowerMonitor->setEnabled(true);
			m_pLineEdit_FlimLaserPowerMonitor->setText(QString::number(m_pDeviceControl->getElforlightLaser()->getLaserPowerLevel()));
			m_pPushButton_FlimLaserPowerIncrease->setEnabled(true);
			m_pPushButton_FlimLaserPowerDecrease->setEnabled(true);
#else
			m_pLineEdit_FlimLaserPowerControl->setEnabled(true);
			m_pPushButton_FlimLaserPowerControl->setEnabled(true);
#endif
		}
		else
			m_pToggleButton_FlimLaserConnect->setChecked(false);
	}
	else
	{
		// Set widgets
		m_pToggleButton_FlimLaserConnect->setText("Connect");
		m_pToggleButton_FlimLaserConnect->setStyleSheet("QPushButton { background-color:#ff0000; }");

		m_pLabel_FlimLaserConnect->setEnabled(true);

		m_pLabel_FlimLaserPowerControl->setDisabled(true);
#ifndef NEXT_GEN_SYSTEM
		m_pLineEdit_FlimLaserPowerMonitor->setDisabled(true);
		m_pLineEdit_FlimLaserPowerMonitor->setText("");
		m_pPushButton_FlimLaserPowerIncrease->setDisabled(true);
		m_pPushButton_FlimLaserPowerDecrease->setDisabled(true);
#else
		m_pLineEdit_FlimLaserPowerControl->setDisabled(true);
		m_pPushButton_FlimLaserPowerControl->setDisabled(true);
#endif

		// Disconnect from rotary motor
		m_pDeviceControl->connectFlimLaser(false);
	}

	return true;
}

void DeviceOptionTab::increaseLaserPower()
{
#ifndef NEXT_GEN_SYSTEM
	static int i = 0;
	if (i++ == 0)
	{
		QMessageBox MsgBox;
		MsgBox.setWindowTitle("Warning");
		MsgBox.setIcon(QMessageBox::Warning);
		MsgBox.setText("Increasing the laser power level can be harmful to the patient.\nWould you like to adjust the laser power level?");
		MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		MsgBox.setDefaultButton(QMessageBox::No);

		int resp = MsgBox.exec();
		switch (resp)
		{
		case QMessageBox::Yes:
			i++;
			break;
		case QMessageBox::No:
			return;
		default:
			return;
		}
	}

	m_pDeviceControl->adjustLaserPower(1);
#endif
}

void DeviceOptionTab::decreaseLaserPower()
{
#ifndef NEXT_GEN_SYSTEM
	m_pDeviceControl->adjustLaserPower(-1);
#endif
}

void DeviceOptionTab::changeFlimLaserPower(const QString &str)
{
#ifndef NEXT_GEN_SYSTEM
	(void)str;
#else

#endif
}

void DeviceOptionTab::applyFlimLaserPower()
{
#ifdef NEXT_GEN_SYSTEM
	m_pConfig->flimLaserPower = m_pLineEdit_FlimLaserPowerControl->text().toInt();
	m_pDeviceControl->adjustLaserPower(m_pConfig->flimLaserPower);

	m_pConfig->writeToLog(QString("FLIm laser power set: %1").arg(m_pConfig->flimLaserPower));
#endif
}


void DeviceOptionTab::connectAxsunControl(bool toggled)
{
	if (toggled)
	{
#ifdef AXSUN_ENABLE
		// Connect to Axsun OCT engine
		if (m_pDeviceControl->connectAxsunControl(toggled))
		{
			// Set widgets		
			m_pToggleButton_AxsunOctConnect->setText("Disconnect");
			m_pToggleButton_AxsunOctConnect->setStyleSheet("QPushButton { background-color:#00ff00; }");

			m_pLabel_LightSource->setEnabled(true);
			m_pToggleButton_LightSource->setEnabled(true);
			m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#ff0000; }");
#ifndef NEXT_GEN_SYSTEM
			m_pLabel_LiveImaging->setEnabled(true);
			m_pToggleButton_LiveImaging->setEnabled(true);
			m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#ff0000; }");
#endif
			m_pLabel_PipelineMode->setEnabled(true);
			m_pRadioButton_JpegCompressed->setEnabled(true);
			m_pRadioButton_RawAdcData->setEnabled(true);
			m_pLabel_BackgroundSubtraction->setEnabled(true);
			m_pPushButton_BgSet->setEnabled(true);
			m_pPushButton_BgReset->setEnabled(true);
			m_pLabel_DispersionCompensation->setEnabled(true);
			m_pLineEdit_DispComp_a2->setEnabled(true);
			m_pLineEdit_DispComp_a3->setEnabled(true);
			m_pPushButton_Compensate->setEnabled(true);

			m_pLabel_VDLLength->setEnabled(true);
			m_pSpinBox_VDLLength->setEnabled(true);
			m_pPushButton_VDLHome->setEnabled(true);
		}
		else
			m_pToggleButton_AxsunOctConnect->setChecked(false);
#else
		m_pToggleButton_AxsunOctConnect->setChecked(false);
#endif
	}
	else
	{
		// Set widgets
		m_pToggleButton_AxsunOctConnect->setText("Connect");
		m_pToggleButton_AxsunOctConnect->setStyleSheet("QPushButton { background-color:#ff0000; }");

		if (m_pToggleButton_LightSource->isChecked()) m_pToggleButton_LightSource->setChecked(false);
		m_pToggleButton_LightSource->setText("On");
		m_pLabel_LightSource->setDisabled(true);
		m_pToggleButton_LightSource->setDisabled(true);
		m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#353535; }");
#ifndef NEXT_GEN_SYSTEM
		if (m_pToggleButton_LiveImaging->isChecked()) m_pToggleButton_LiveImaging->setChecked(false);
		m_pToggleButton_LiveImaging->setText("On");
		m_pLabel_LiveImaging->setDisabled(true);
		m_pToggleButton_LiveImaging->setDisabled(true);
		m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#353535; }");
#endif
		m_pLabel_PipelineMode->setDisabled(true);
		m_pRadioButton_JpegCompressed->setDisabled(true);
		m_pRadioButton_RawAdcData->setDisabled(true);
		m_pLabel_BackgroundSubtraction->setDisabled(true);
		m_pPushButton_BgSet->setDisabled(true);
		m_pPushButton_BgReset->setDisabled(true);
		m_pLabel_DispersionCompensation->setDisabled(true);
		m_pLineEdit_DispComp_a2->setDisabled(true);
		m_pLineEdit_DispComp_a3->setDisabled(true);
		m_pPushButton_Compensate->setDisabled(true);
		
		m_pLabel_VDLLength->setDisabled(true);
		m_pSpinBox_VDLLength->setDisabled(true);
		m_pPushButton_VDLHome->setDisabled(true);
		
#ifdef AXSUN_ENABLE
		// Disconnect from Axsun OCT engine
		m_pDeviceControl->connectAxsunControl(toggled);
#endif
	}
}

void DeviceOptionTab::setLightSource(bool toggled)
{
	if (toggled)
	{
#ifdef AXSUN_ENABLE
		// Start Axsun light source operation
		m_pDeviceControl->setLightSource(true);
#endif

		// Set widgets
		m_pToggleButton_LightSource->setText("Off");
		m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#00ff00; }");
	}
	else
	{
		// Set widgets
		m_pToggleButton_LightSource->setText("On");
		m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#ff0000; }");

#ifdef AXSUN_ENABLE
		// Stop Axsun light source operation
		m_pDeviceControl->setLightSource(false);
#endif
	}
}

void DeviceOptionTab::setLiveImaging(bool toggled)
{
#ifndef NEXT_GEN_SYSTEM
	if (toggled)
	{
#ifdef AXSUN_ENABLE
		// Start Axsun light source operation
		m_pDeviceControl->setLiveImaging(true);
#endif

		// Set widgets
		m_pToggleButton_LiveImaging->setText("Off");
		m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#00ff00; }");
	}
	else
	{
		// Set widgets
		m_pToggleButton_LiveImaging->setText("On");
		m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#ff0000; }");

#ifdef AXSUN_ENABLE
		// Stop Axsun light source operation
		m_pDeviceControl->setLiveImaging(false);
#endif
	}
#else
	(void)toggled;
#endif
}

void DeviceOptionTab::setPipelineMode(int id)
{
#ifdef AXSUN_ENABLE
	m_pDeviceControl->setPipelineMode(id);
#else
	(void)id;
#endif
}

void DeviceOptionTab::setBackground()
{
#ifdef AXSUN_ENABLE
	m_pDeviceControl->getAxsunControl()->setPipelineMode(AxPipelineMode::SQRT);
	m_pDeviceControl->setBackground();
	m_pDeviceControl->getAxsunControl()->setPipelineMode(AxPipelineMode::JPEG_COMP);
#endif
			
	///QFile file("bg.data");
	///if (file.open(QIODevice::WriteOnly))
	///	file.write(reinterpret_cast<const char*>(getDeviceControl()->getAxsunControl()->background_vector.data()),
	///		sizeof(uint16_t) * getDeviceControl()->getAxsunControl()->background_vector.size());
	///file.close();
}

void DeviceOptionTab::resetBackground()
{
#ifdef AXSUN_ENABLE
	memset(m_pDeviceControl->getAxsunControl()->background_frame, 0, sizeof(uint16_t) * m_pDeviceControl->getAxsunControl()->background_frame.length());
	m_pDeviceControl->setBackground();
#endif
}

void DeviceOptionTab::setDispersionCompensation()
{
	m_pConfig->axsunDispComp_a2 = m_pLineEdit_DispComp_a2->text().toFloat();
	m_pConfig->axsunDispComp_a3 = m_pLineEdit_DispComp_a3->text().toFloat();

#ifdef AXSUN_ENABLE
	m_pDeviceControl->setDispersionCompensation(m_pConfig->axsunDispComp_a2, m_pConfig->axsunDispComp_a3);
#endif
}

void DeviceOptionTab::setClockDelay(double)
{
#ifdef AXSUN_ENABLE
	m_pDeviceControl->setClockDelay(CLOCK_DELAY);
#endif
}

void DeviceOptionTab::setVDLLength(double length)
{
#ifdef AXSUN_ENABLE
	m_pDeviceControl->setVDLLength(length);
	if (m_pStreamTab)
		m_pStreamTab->getCalibScrollBar()->setValue(int(length * 100.0));
#else
	(void)length;
#endif
}

void DeviceOptionTab::setVDLHome()
{
#ifdef AXSUN_ENABLE
	m_pDeviceControl->setVDLHome();
#endif
	m_pSpinBox_VDLLength->setValue(0.0);
}

void DeviceOptionTab::setVDLWidgets(bool enabled)
{
	m_pLabel_VDLLength->setEnabled(enabled);
	m_pSpinBox_VDLLength->setEnabled(enabled);
	m_pPushButton_VDLHome->setEnabled(enabled);
}
