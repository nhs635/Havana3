
#include "DeviceOptionTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QResultTab.h>

#include <Havana3/Dialog/SettingDlg.h>

#include <DeviceControl/DeviceControl.h>
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


DeviceOptionTab::DeviceOptionTab(QWidget *parent) :
    QDialog(parent), m_pPatientSummaryTab(nullptr), m_pStreamTab(nullptr), m_pResultTab(nullptr), m_pDeviceControl(nullptr)
{
    // Set configuration objects
	QString parent_name, parent_title = parent->windowTitle();
	if (parent_title.contains("Summary"))
	{
		parent_name = "Summary";
		m_pPatientSummaryTab = dynamic_cast<QPatientSummaryTab*>(parent);
		m_pConfig = m_pPatientSummaryTab->getMainWnd()->m_pConfiguration;
		m_pDeviceControl = new DeviceControl(m_pConfig);
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
		m_pConfig = m_pResultTab->getMainWnd()->m_pConfiguration;
		m_pDeviceControl = new DeviceControl(m_pConfig);
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

	m_pLineEdit_RPM = new QLineEdit(this);
	m_pLineEdit_RPM->setFixedWidth(40);
	m_pLineEdit_RPM->setText(QString::number(m_pConfig->rotaryRpm));
	m_pLineEdit_RPM->setAlignment(Qt::AlignCenter);
	m_pLineEdit_RPM->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_RPM->setDisabled(true);

	m_pLabel_RotationSpeed = new QLabel(this);
	m_pLabel_RotationSpeed->setText("Rotation Speed");
	m_pLabel_RotationSpeed->setDisabled(true);

	m_pLabel_RPM = new QLabel("RPM", this);
	m_pLabel_RPM->setBuddy(m_pLineEdit_RPM);
	m_pLabel_RPM->setDisabled(true);

	m_pToggleButton_Rotate = new QPushButton(this);
	m_pToggleButton_Rotate->setText("Rotate");
	m_pToggleButton_Rotate->setFixedWidth(100);
	m_pToggleButton_Rotate->setCheckable(true);
	m_pToggleButton_Rotate->setDisabled(true);

	// Create widgets for pullback stage control
	m_pToggleButton_PullbackConnect = new QPushButton(this);
	m_pToggleButton_PullbackConnect->setText("Connect");
	m_pToggleButton_PullbackConnect->setFixedWidth(100);
	m_pToggleButton_PullbackConnect->setCheckable(true);
	m_pToggleButton_PullbackConnect->setStyleSheet("QPushButton { background-color:#ff0000; }");
	
	m_pLabel_PullbackConnect = new QLabel(this);
	m_pLabel_PullbackConnect->setText("Pullback Motor");

	m_pLineEdit_PullbackSpeed = new QLineEdit(this);
	m_pLineEdit_PullbackSpeed->setFixedWidth(35);
	m_pLineEdit_PullbackSpeed->setText(QString::number(m_pConfig->pullbackSpeed));
	m_pLineEdit_PullbackSpeed->setAlignment(Qt::AlignCenter);
	m_pLineEdit_PullbackSpeed->setDisabled(true);
	m_pLabel_PullbackSpeed = new QLabel("Pullback Speed", this);
	m_pLabel_PullbackSpeed->setBuddy(m_pLineEdit_PullbackSpeed);
	m_pLabel_PullbackSpeed->setDisabled(true);
	m_pLabel_PullbackSpeedUnit = new QLabel("mm/s  ", this);
	m_pLabel_PullbackSpeedUnit->setDisabled(true);

	m_pLineEdit_PullbackLength = new QLineEdit(this);
	m_pLineEdit_PullbackLength->setFixedWidth(35);
	m_pLineEdit_PullbackLength->setText(QString::number(m_pConfig->pullbackLength));
	m_pLineEdit_PullbackLength->setAlignment(Qt::AlignCenter);
	m_pLineEdit_PullbackLength->setDisabled(true);
	m_pLabel_PullbackLength = new QLabel("Pullback Length", this);
	m_pLabel_PullbackLength->setBuddy(m_pLineEdit_PullbackLength);
	m_pLabel_PullbackLength->setDisabled(true);
	m_pLabel_PullbackLengthUnit = new QLabel("mm", this);
	m_pLabel_PullbackLengthUnit->setDisabled(true);

	m_pPushButton_Pullback = new QPushButton(this);
	m_pPushButton_Pullback->setText("Pullback");
	m_pPushButton_Pullback->setFixedWidth(100);
	m_pPushButton_Pullback->setDisabled(true);
	m_pPushButton_Home = new QPushButton(this);
	m_pPushButton_Home->setText("Home");
	m_pPushButton_Home->setFixedWidth(48);
	m_pPushButton_Home->setDisabled(true);
	m_pPushButton_PullbackStop = new QPushButton(this);
	m_pPushButton_PullbackStop->setText("Stop");
	m_pPushButton_PullbackStop->setFixedWidth(48);
	m_pPushButton_PullbackStop->setDisabled(true);
	
	// Set Layout
	QGridLayout *pGridLayout_RotaryMotor = new QGridLayout;
	pGridLayout_RotaryMotor->setSpacing(3);

	pGridLayout_RotaryMotor->addWidget(m_pLabel_RotaryConnect, 0, 0);
	pGridLayout_RotaryMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 1, 1, 3);
	pGridLayout_RotaryMotor->addWidget(m_pToggleButton_RotaryConnect, 0, 4);

	pGridLayout_RotaryMotor->addWidget(m_pLabel_RotationSpeed, 1, 0);
	pGridLayout_RotaryMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_RotaryMotor->addWidget(m_pLineEdit_RPM, 1, 2);
	pGridLayout_RotaryMotor->addWidget(m_pLabel_RPM, 1, 3);
	pGridLayout_RotaryMotor->addWidget(m_pToggleButton_Rotate, 1, 4);

	QGridLayout *pGridLayout_PullbackMotor = new QGridLayout;
	pGridLayout_PullbackMotor->setSpacing(3);

	pGridLayout_PullbackMotor->addWidget(m_pLabel_PullbackConnect, 0, 0);
	pGridLayout_PullbackMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 1, 1, 3);
	pGridLayout_PullbackMotor->addWidget(m_pToggleButton_PullbackConnect, 0, 4, 1, 2);

	pGridLayout_PullbackMotor->addWidget(m_pLabel_PullbackSpeed, 1, 0);
	pGridLayout_PullbackMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 1);
	pGridLayout_PullbackMotor->addWidget(m_pLineEdit_PullbackSpeed, 1, 2, Qt::AlignRight);
	pGridLayout_PullbackMotor->addWidget(m_pLabel_PullbackSpeedUnit, 1, 3);
	pGridLayout_PullbackMotor->addWidget(m_pPushButton_Pullback, 1, 4, 1, 2);

	pGridLayout_PullbackMotor->addWidget(m_pLabel_PullbackLength, 2, 0);
	pGridLayout_PullbackMotor->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 1);
	pGridLayout_PullbackMotor->addWidget(m_pLineEdit_PullbackLength, 2, 2, Qt::AlignRight);
	pGridLayout_PullbackMotor->addWidget(m_pLabel_PullbackLengthUnit, 2, 3);
	pGridLayout_PullbackMotor->addWidget(m_pPushButton_Home, 2, 4);
	pGridLayout_PullbackMotor->addWidget(m_pPushButton_PullbackStop, 2, 5, Qt::AlignRight);


	QVBoxLayout *pVBoxLayout_HelicalScanning = new QVBoxLayout;
	pVBoxLayout_HelicalScanning->setSpacing(10);
	
	pVBoxLayout_HelicalScanning->addItem(pGridLayout_RotaryMotor);
	pVBoxLayout_HelicalScanning->addItem(pGridLayout_PullbackMotor);

	pGroupBox_HelicalScanning->setLayout(pVBoxLayout_HelicalScanning);

	m_pVBoxLayout_DeviceOption->addWidget(pGroupBox_HelicalScanning);
	
	// Connect signal and slot
	connect(m_pToggleButton_RotaryConnect, SIGNAL(toggled(bool)), this, SLOT(connectRotaryMotor(bool)));
	connect(m_pLineEdit_RPM, SIGNAL(textChanged(const QString &)), this, SLOT(changeRotaryRpm(const QString &)));
	connect(m_pToggleButton_Rotate, SIGNAL(toggled(bool)), this, SLOT(rotate(bool)));

	connect(m_pToggleButton_PullbackConnect, SIGNAL(toggled(bool)), this, SLOT(connectPullbackMotor(bool)));
	connect(m_pLineEdit_PullbackSpeed, SIGNAL(textChanged(const QString &)), this, SLOT(setTargetSpeed(const QString &)));
	connect(m_pLineEdit_PullbackLength, SIGNAL(textChanged(const QString &)), this, SLOT(changePullbackLength(const QString &)));
	connect(m_pPushButton_Pullback, SIGNAL(clicked(bool)), this, SLOT(moveAbsolute()));
	connect(m_pPushButton_Home, SIGNAL(clicked(bool)), this, SLOT(home()));
	connect(m_pPushButton_PullbackStop, SIGNAL(clicked(bool)), this, SLOT(stop()));
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
	//m_pToggleButton_AsynchronizedPulsedLaser->setDisabled(true);

	m_pLabel_AsynchronizedPulsedLaser = new QLabel("Asynchronized UV Pulsed Laser", this);
	m_pLabel_AsynchronizedPulsedLaser->setBuddy(m_pToggleButton_AsynchronizedPulsedLaser);
	//m_pLabel_AsynchronizedPulsedLaser->setDisabled(true);

	m_pToggleButton_SynchronizedPulsedLaser = new QPushButton(this);
	m_pToggleButton_SynchronizedPulsedLaser->setText("On");
	m_pToggleButton_SynchronizedPulsedLaser->setFixedWidth(40);
	m_pToggleButton_SynchronizedPulsedLaser->setCheckable(true);
	m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");
	//m_pToggleButton_SynchronizedPulsedLaser->setDisabled(true);

	m_pLabel_SynchronizedPulsedLaser = new QLabel("Synchronized UV Pulsed Laser", this);
	m_pLabel_SynchronizedPulsedLaser->setBuddy(m_pToggleButton_SynchronizedPulsedLaser);
	//m_pLabel_SynchronizedPulsedLaser->setDisabled(true);
	
	// Create widgets for PMT gain control
	m_pLineEdit_PmtGainVoltage = new QLineEdit(this);
	m_pLineEdit_PmtGainVoltage->setFixedWidth(45);
	m_pLineEdit_PmtGainVoltage->setText(QString::number(m_pConfig->pmtGainVoltage, 'f', 3));
	m_pLineEdit_PmtGainVoltage->setAlignment(Qt::AlignCenter);
	m_pLineEdit_PmtGainVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	//m_pLineEdit_PmtGainVoltage->setDisabled(true);

	m_pLabel_PmtGainControl = new QLabel("PMT Gain Voltage", this);
	m_pLabel_PmtGainControl->setBuddy(m_pLineEdit_PmtGainVoltage);
	//m_pLabel_PmtGainControl->setDisabled(true);

	m_pLabel_PmtGainVoltage = new QLabel("V  ", this);
	m_pLabel_PmtGainVoltage->setBuddy(m_pLineEdit_PmtGainVoltage);
	//m_pLabel_PmtGainVoltage->setDisabled(true);

	m_pToggleButton_PmtGainVoltage = new QPushButton(this);
	m_pToggleButton_PmtGainVoltage->setText("On");	
	m_pToggleButton_PmtGainVoltage->setFixedWidth(40);
	m_pToggleButton_PmtGainVoltage->setCheckable(true);
	m_pToggleButton_PmtGainVoltage->setStyleSheet("QPushButton { background-color:#ff0000; }");
	//m_pToggleButton_PmtGainVoltage->setDisabled(true);
	
	// Create widgets for FLIM laser power control
	m_pToggleButton_FlimLaserConnect = new QPushButton(this);
	m_pToggleButton_FlimLaserConnect->setText("Connect");
	m_pToggleButton_FlimLaserConnect->setFixedWidth(100);
	m_pToggleButton_FlimLaserConnect->setCheckable(true);
	m_pToggleButton_FlimLaserConnect->setStyleSheet("QPushButton { background-color:#ff0000; }");
	
	m_pLabel_FlimLaserConnect = new QLabel(this);
	m_pLabel_FlimLaserConnect->setText("FLIm UV Pulsed Laser");

#ifndef NEXT_GEN_SYSTEM
	m_pSpinBox_FlimLaserPowerControl = new QSpinBox(this);
	m_pSpinBox_FlimLaserPowerControl->setValue(0);
	m_pSpinBox_FlimLaserPowerControl->setRange(-10000, 10000);
	m_pSpinBox_FlimLaserPowerControl->setFixedWidth(15);
	m_pSpinBox_FlimLaserPowerControl->setDisabled(true);
	static int flim_laser_power_level = 0;
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
	pGridLayout_FlimLaser->addWidget(m_pToggleButton_FlimLaserConnect, 0, 3, 1, 2);

	pGridLayout_FlimLaser->addWidget(m_pLabel_FlimLaserPowerControl, 1, 0);
	pGridLayout_FlimLaser->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 1, 1, 2);
#ifndef NEXT_GEN_SYSTEM
	pGridLayout_FlimLaser->addWidget(m_pSpinBox_FlimLaserPowerControl, 1, 3, 1, 2, Qt::AlignRight);
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
	connect(m_pSpinBox_FlimLaserPowerControl, SIGNAL(valueChanged(int)), this, SLOT(adjustLaserPower(int)));
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
	
	pGridLayout_AxsunControl->addWidget(m_pLabel_VDLLength, 3, 0);
	pGridLayout_AxsunControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 3, 1);
	pGridLayout_AxsunControl->addWidget(m_pSpinBox_VDLLength, 3, 2);
	pGridLayout_AxsunControl->addWidget(m_pPushButton_VDLHome, 3, 3);


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
	connect(m_pSpinBox_VDLLength, SIGNAL(valueChanged(double)), this, SLOT(setVDLLength(double)));
	connect(m_pPushButton_VDLHome, SIGNAL(clicked(bool)), this, SLOT(setVDLHome()));
//	connect(this, SIGNAL(transferAxsunArray(int)), m_pStreamTab->getOperationTab()->getProgressBar(), SLOT(setValue(int)));
}


bool DeviceOptionTab::connectRotaryMotor(bool toggled)
{
	if (toggled)
	{
		// Connect to rotary motor
		if (m_pStreamTab || m_pDeviceControl->connectRotaryMotor(true))
		{
			// Set widgets
			m_pToggleButton_RotaryConnect->setText("Disconnect");
			m_pToggleButton_RotaryConnect->setStyleSheet("QPushButton { background-color:#00ff00; }");

			m_pLabel_RotaryConnect->setDisabled(true);

			m_pLabel_RotationSpeed->setEnabled(true);
			m_pLineEdit_RPM->setEnabled(true);
			m_pLabel_RPM->setEnabled(true);
			m_pToggleButton_Rotate->setEnabled(true);
			m_pToggleButton_Rotate->setStyleSheet("QPushButton { background-color:#ff0000; }");
		}
		else
			m_pToggleButton_RotaryConnect->setChecked(false);
	}
	else
	{
		// Set widgets
		m_pToggleButton_RotaryConnect->setText("Connect");
		m_pToggleButton_RotaryConnect->setStyleSheet("QPushButton { background-color:#ff0000; }");

		m_pLabel_RotaryConnect->setEnabled(true);

		m_pLabel_RotationSpeed->setDisabled(true);
		m_pLineEdit_RPM->setDisabled(true);
		m_pLabel_RPM->setDisabled(true);
		m_pToggleButton_Rotate->setDisabled(true);
		m_pToggleButton_Rotate->setStyleSheet("QPushButton { background-color:#353535; }");

		// Disconnect from rotary motor
		m_pDeviceControl->connectRotaryMotor(false);
	}

	return true;
}

void DeviceOptionTab::rotate(bool toggled)
{
	m_pDeviceControl->rotate(toggled);
	
	// Set widgets
	m_pLineEdit_RPM->setDisabled(toggled);
	m_pLabel_RPM->setDisabled(toggled);

	m_pToggleButton_Rotate->setStyleSheet(toggled ? "QPushButton { background-color:#00ff00; }" : "QPushButton { background-color:#ff0000; }");
	m_pToggleButton_Rotate->setText(toggled ? "Stop" : "Rotate");
}

void DeviceOptionTab::changeRotaryRpm(const QString &str)
{
	m_pConfig->rotaryRpm = str.toInt();

	m_pConfig->writeToLog(QString("Rotation RPM set: %1 rpm").arg(m_pConfig->rotaryRpm));
}


bool DeviceOptionTab::connectPullbackMotor(bool toggled)
{
	if (toggled)
	{
		// Connect to pullback motor
		if (m_pStreamTab || m_pDeviceControl->connectPullbackMotor(true))
		{
			// Set widgets
			m_pToggleButton_PullbackConnect->setText("Disconnect");
			m_pToggleButton_PullbackConnect->setStyleSheet("QPushButton { background-color:#00ff00; }");

			m_pLabel_PullbackConnect->setDisabled(true);

			m_pLabel_PullbackSpeed->setEnabled(true);
			m_pLineEdit_PullbackSpeed->setEnabled(true);
			m_pLabel_PullbackSpeedUnit->setEnabled(true);

			m_pLabel_PullbackLength->setEnabled(true);
			m_pLineEdit_PullbackLength->setEnabled(true);
			m_pLabel_PullbackLengthUnit->setEnabled(true);

			m_pPushButton_Pullback->setEnabled(true);
			m_pPushButton_Home->setEnabled(true);
			m_pPushButton_PullbackStop->setEnabled(true);
			m_pPushButton_Pullback->setStyleSheet("QPushButton { background-color:#ff0000; }");
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

		m_pLabel_PullbackSpeed->setDisabled(true);
		m_pLineEdit_PullbackSpeed->setDisabled(true);
		m_pLabel_PullbackSpeedUnit->setDisabled(true);

		m_pLabel_PullbackLength->setDisabled(true);
		m_pLineEdit_PullbackLength->setDisabled(true);
		m_pLabel_PullbackLengthUnit->setDisabled(true);

		m_pPushButton_Pullback->setDisabled(true);
		m_pPushButton_Home->setDisabled(true);
		m_pPushButton_PullbackStop->setDisabled(true);
		m_pPushButton_Pullback->setStyleSheet("QPushButton { background-color:#353535; }");

		// Disconnect from rotary motor
		m_pDeviceControl->connectRotaryMotor(false);
	}
		
	return true;
}

void DeviceOptionTab::moveAbsolute()
{
	m_pDeviceControl->pullback();

	// Set widgets
	m_pLabel_PullbackSpeed->setDisabled(true);
	m_pLabel_PullbackSpeedUnit->setDisabled(true);
	m_pLineEdit_PullbackSpeed->setDisabled(true);

	m_pLabel_PullbackLength->setDisabled(true);
	m_pLabel_PullbackLengthUnit->setDisabled(true);
	m_pLineEdit_PullbackLength->setDisabled(true);

	m_pPushButton_Home->setDisabled(true);
	
	m_pPushButton_Pullback->setStyleSheet("QPushButton { background-color:#00ff00; }");
}

void DeviceOptionTab::setTargetSpeed(const QString & str)
{
	m_pConfig->pullbackSpeed = str.toFloat();

	m_pConfig->writeToLog(QString("Pullback speed set: %1 mm/s").arg(m_pConfig->pullbackSpeed, 3, 'f', 2));
}

void DeviceOptionTab::changePullbackLength(const QString &str)
{
	m_pConfig->pullbackLength = str.toInt();

	m_pConfig->writeToLog(QString("Pullback length set: %1 mm").arg(m_pConfig->pullbackLength, 3, 'f', 2));
}

void DeviceOptionTab::home()
{
	m_pDeviceControl->home();
}

void DeviceOptionTab::stop()
{
	m_pDeviceControl->stop();

//	// Stop rotary motor
//	if (m_pToggleButton_Rotate->isChecked())
//	{
//		m_pToggleButton_Rotate->setChecked(false);

//		this->setChecked(false);
//		this->setChecked(true);
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


void DeviceOptionTab::startFlimAsynchronization(bool toggled)
{
#ifdef NI_ENABLE
	if (toggled)
	{
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
#else
	(void)toggled;
#endif
}

void DeviceOptionTab::startFlimSynchronization(bool toggled)
{
#ifdef NI_ENABLE
	if (toggled)
	{
		// Start synchronous FLIm-OCT operation
		if (m_pStreamTab || m_pDeviceControl->startSynchronization(true))
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
	}
	else
	{
		// Set widgets
		m_pToggleButton_SynchronizedPulsedLaser->setText("On");
		m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");

		m_pLabel_AsynchronizedPulsedLaser->setEnabled(true);
		m_pToggleButton_AsynchronizedPulsedLaser->setEnabled(true);
		m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");

		// Stop synchronous FLIm-OCT operation
		m_pDeviceControl->startSynchronization(false);
	}
#else
	(void)toggled;
#endif
}


void DeviceOptionTab::applyPmtGainVoltage(bool toggled)
{
#ifdef NI_ENABLE	
	if (toggled)
	{
		// Apply PMT gain voltage
		if (m_pDeviceControl->applyPmtGainVoltage(true)) // m_pStreamTab || 
		{
			// Set widgets
			m_pToggleButton_PmtGainVoltage->setText("Off");
			m_pToggleButton_PmtGainVoltage->setStyleSheet("QPushButton { background-color:#00ff00; }");

			m_pLineEdit_PmtGainVoltage->setDisabled(true);
			m_pLabel_PmtGainVoltage->setDisabled(true);
		}
		else
			m_pToggleButton_PmtGainVoltage->setChecked(false);
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
#else
	(void)toggled;
#endif
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
		if (m_pStreamTab || m_pDeviceControl->connectFlimLaser(true))
		{
			// Set widgets
			m_pToggleButton_FlimLaserConnect->setText("Disconnect");
			m_pToggleButton_FlimLaserConnect->setStyleSheet("QPushButton { background-color:#00ff00; }");

			m_pLabel_FlimLaserConnect->setDisabled(true);

			m_pLabel_FlimLaserPowerControl->setEnabled(true);
#ifndef NEXT_GEN_SYSTEM
			m_pSpinBox_FlimLaserPowerControl->setEnabled(true);
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
		m_pSpinBox_FlimLaserPowerControl->setDisabled(true);
#else
		m_pLineEdit_FlimLaserPowerControl->setDisabled(true);
		m_pPushButton_FlimLaserPowerControl->setDisabled(true);
#endif

		// Disconnect from rotary motor
		m_pDeviceControl->connectFlimLaser(false);
	}

	return true;
}

void DeviceOptionTab::adjustLaserPower(int level)
{
	static int i = 0;

	if (i == 0)
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

	static int flim_laser_power_level = 0;

	if (level > flim_laser_power_level)
	{
		m_pDeviceControl->adjustLaserPower(level);
		flim_laser_power_level++;
	}
	else
	{
		m_pDeviceControl->adjustLaserPower(level);
		flim_laser_power_level--;
	}
}

void DeviceOptionTab::changeFlimLaserPower(const QString &str)
{
	(void)str;
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
		// Connect to Axsun OCT engine
		if (m_pStreamTab || m_pDeviceControl->connectAxsunControl(toggled))
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

			m_pLabel_VDLLength->setEnabled(true);
			m_pSpinBox_VDLLength->setEnabled(true);
			m_pPushButton_VDLHome->setEnabled(true);
		}
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
		
		m_pLabel_VDLLength->setDisabled(true);
		m_pSpinBox_VDLLength->setDisabled(true);
		m_pPushButton_VDLHome->setDisabled(true);
	}
}

void DeviceOptionTab::setLightSource(bool toggled)
{
	if (toggled)
	{
		// Start Axsun light source operation
		m_pDeviceControl->setLightSource(true);

		// Set widgets
		m_pToggleButton_LightSource->setText("Off");
		m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#00ff00; }");
	}
	else
	{
		// Set widgets
		m_pToggleButton_LightSource->setText("On");
		m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#ff0000; }");

		// Stop Axsun light source operation
		m_pDeviceControl->setLightSource(false);
	}
}

void DeviceOptionTab::setLiveImaging(bool toggled)
{
#ifndef NEXT_GEN_SYSTEM
	if (toggled)
	{
		// Start Axsun light source operation
		m_pDeviceControl->setLiveImaging(true);

		// Set widgets
		m_pToggleButton_LiveImaging->setText("Off");
		m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#00ff00; }");
	}
	else
	{
		// Set widgets
		m_pToggleButton_LiveImaging->setText("On");
		m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#ff0000; }");

		// Stop Axsun light source operation
		m_pDeviceControl->setLiveImaging(false);
	}
#else
	(void)toggled;
#endif
}

void DeviceOptionTab::setClockDelay(double)
{
	m_pDeviceControl->setClockDelay(CLOCK_DELAY);
}

void DeviceOptionTab::setVDLLength(double length)
{
	m_pDeviceControl->setVDLLength(length);
	m_pStreamTab->getCalibScrollBar()->setValue(int(length * 100.0));
}

void DeviceOptionTab::setVDLHome()
{
	m_pDeviceControl->setVDLHome();
	m_pSpinBox_VDLLength->setValue(0.0);
}

void DeviceOptionTab::setVDLWidgets(bool enabled)
{
	m_pLabel_VDLLength->setEnabled(enabled);
//	m_pSpinBox_VDLLength->setEnabled(enabled);
	m_pPushButton_VDLHome->setEnabled(enabled);
}
