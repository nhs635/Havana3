
#include "QDeviceControlTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QOperationTab.h>
#include <Havana3/QVisualizationTab.h>

#include <Havana3/Dialog/FlimCalibDlg.h>
#include <Havana3/Viewer/QImageView.h>

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


QDeviceControlTab::QDeviceControlTab(QWidget *parent) :
    QDialog(parent), 
	m_pPullbackMotor(nullptr), m_pRotaryMotor(nullptr),
	m_pFlimFreqDivider(nullptr), m_pAxsunFreqDivider(nullptr), m_pPmtGainControl(nullptr),
	m_pElforlightLaser(nullptr), m_pFlimCalibDlg(nullptr),
    m_pAxsunControl(nullptr)
{
	// Set main window objects
    m_pStreamTab = dynamic_cast<QStreamTab*>(parent);
    m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;

    // Create layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(3);

	createHelicalScanningControl();
	createFlimSystemControl();
    createAxsunOctSystemControl();
	
    // Set layout
    setLayout(m_pVBoxLayout);
}

QDeviceControlTab::~QDeviceControlTab()
{
	if (m_pFlimCalibDlg) m_pFlimCalibDlg->close();
	//setControlsStatus();
}

void QDeviceControlTab::closeEvent(QCloseEvent* e)
{
	e->accept();
}


void QDeviceControlTab::setControlsStatus()
{
	if (m_pFlimCalibDlg) m_pFlimCalibDlg->close();

	if (m_pGroupBox_HelicalScanningControl->isChecked()) m_pGroupBox_HelicalScanningControl->setChecked(false);
	if (m_pGroupBox_FlimControl->isChecked()) m_pGroupBox_FlimControl->setChecked(false);
	if (m_pGroupBox_AxsunOctControl->isChecked()) m_pGroupBox_AxsunOctControl->setChecked(false);
}


void QDeviceControlTab::createHelicalScanningControl()
{
	// Helical Scanning Control GroupBox
	m_pGroupBox_HelicalScanningControl = new QGroupBox;
	m_pGroupBox_HelicalScanningControl->setTitle("Helical Scanning Control  ");
	m_pGroupBox_HelicalScanningControl->setCheckable(true);
	m_pGroupBox_HelicalScanningControl->setChecked(false);
	m_pGroupBox_HelicalScanningControl->setStyleSheet("QGroupBox { padding-top: 18px; margin-top: -10px; } QGroupBox::title { subcontrol-origin: margin; left: 8px; top: 2px; }");

	QGridLayout *pGridLayout_HelicalScanningControl = new QGridLayout;
	pGridLayout_HelicalScanningControl->setSpacing(2);

	// Create widgets for pullback stage control
	m_pLineEdit_PullbackSpeed = new QLineEdit(m_pGroupBox_HelicalScanningControl);
	m_pLineEdit_PullbackSpeed->setFixedWidth(25);
	m_pLineEdit_PullbackSpeed->setText(QString::number(m_pConfig->pullbackSpeed));
	m_pLineEdit_PullbackSpeed->setAlignment(Qt::AlignCenter);
	m_pLineEdit_PullbackSpeed->setDisabled(true);
	m_pLabel_PullbackSpeed = new QLabel("Pullback Speed", m_pGroupBox_HelicalScanningControl);
	m_pLabel_PullbackSpeed->setBuddy(m_pLineEdit_PullbackSpeed);
	m_pLabel_PullbackSpeed->setDisabled(true);
	m_pLabel_PullbackSpeedUnit = new QLabel("mm/s", m_pGroupBox_HelicalScanningControl);
	m_pLabel_PullbackSpeedUnit->setDisabled(true);

	m_pLineEdit_PullbackLength = new QLineEdit(m_pGroupBox_HelicalScanningControl);
	m_pLineEdit_PullbackLength->setFixedWidth(25);
	m_pLineEdit_PullbackLength->setText(QString::number(m_pConfig->pullbackLength));
	m_pLineEdit_PullbackLength->setAlignment(Qt::AlignCenter);
	m_pLineEdit_PullbackLength->setDisabled(true);
	m_pLabel_PullbackLength = new QLabel("Pullback Length", m_pGroupBox_HelicalScanningControl);
	m_pLabel_PullbackLength->setBuddy(m_pLineEdit_PullbackLength);
	m_pLabel_PullbackLength->setDisabled(true);
	m_pLabel_PullbackLengthUnit = new QLabel("mm", m_pGroupBox_HelicalScanningControl);
	m_pLabel_PullbackLengthUnit->setDisabled(true);

	m_pPushButton_Pullback = new QPushButton(m_pGroupBox_HelicalScanningControl);
	m_pPushButton_Pullback->setText("Pullback");
	m_pPushButton_Pullback->setDisabled(true);
	m_pPushButton_Home = new QPushButton(m_pGroupBox_HelicalScanningControl);
	m_pPushButton_Home->setText("Home");
	m_pPushButton_Home->setFixedWidth(40);
	m_pPushButton_Home->setDisabled(true);
	m_pPushButton_PullbackStop = new QPushButton(m_pGroupBox_HelicalScanningControl);
	m_pPushButton_PullbackStop->setText("Stop");
	m_pPushButton_PullbackStop->setFixedWidth(40);
	m_pPushButton_PullbackStop->setDisabled(true);

	// Create widgets for Faulhaber motor control	
	m_pLineEdit_RPM = new QLineEdit(m_pGroupBox_HelicalScanningControl);
	m_pLineEdit_RPM->setFixedWidth(35);
	m_pLineEdit_RPM->setText(QString::number(m_pConfig->rotaryRpm));
	m_pLineEdit_RPM->setAlignment(Qt::AlignCenter);
	m_pLineEdit_RPM->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_RPM->setDisabled(true);
	m_pLabel_RotationSpeed = new QLabel(m_pGroupBox_HelicalScanningControl);
	m_pLabel_RotationSpeed->setText("Rotation Speed");
	m_pLabel_RotationSpeed->setDisabled(true);
	m_pLabel_RPM = new QLabel("RPM", m_pGroupBox_HelicalScanningControl);
	m_pLabel_RPM->setBuddy(m_pLineEdit_RPM);
	m_pLabel_RPM->setDisabled(true);

	m_pToggleButton_Rotate = new QPushButton(m_pGroupBox_HelicalScanningControl);
	m_pToggleButton_Rotate->setText("Rotate");
	m_pToggleButton_Rotate->setCheckable(true);
	m_pToggleButton_Rotate->setDisabled(true);

	// Set Layout
	QLabel *pLabel_Null = new QLabel(m_pGroupBox_HelicalScanningControl);
	pLabel_Null->setFixedWidth(50);

	pGridLayout_HelicalScanningControl->addWidget(pLabel_Null, 0, 0);
	pGridLayout_HelicalScanningControl->addWidget(m_pLabel_PullbackSpeed, 0, 1);
	pGridLayout_HelicalScanningControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 2);
	pGridLayout_HelicalScanningControl->addWidget(m_pLineEdit_PullbackSpeed, 0, 3, Qt::AlignRight);
	pGridLayout_HelicalScanningControl->addWidget(m_pLabel_PullbackSpeedUnit, 0, 4);
	pGridLayout_HelicalScanningControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 5);
	pGridLayout_HelicalScanningControl->addWidget(m_pPushButton_Pullback, 0, 6, 1, 2);

	pGridLayout_HelicalScanningControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_HelicalScanningControl->addWidget(m_pLabel_PullbackLength, 1, 1);
	pGridLayout_HelicalScanningControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 2);
	pGridLayout_HelicalScanningControl->addWidget(m_pLineEdit_PullbackLength, 1, 3, Qt::AlignRight);
	pGridLayout_HelicalScanningControl->addWidget(m_pLabel_PullbackLengthUnit, 1, 4);
	pGridLayout_HelicalScanningControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 5);
	pGridLayout_HelicalScanningControl->addWidget(m_pPushButton_Home, 1, 6);
	pGridLayout_HelicalScanningControl->addWidget(m_pPushButton_PullbackStop, 1, 7);

	QLabel *pLabel_Null2 = new QLabel(m_pGroupBox_HelicalScanningControl);
	pLabel_Null2->setFixedHeight(2);
	pGridLayout_HelicalScanningControl->addWidget(pLabel_Null2, 2, 0, 1, 8);

	pGridLayout_HelicalScanningControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 3, 0);
	pGridLayout_HelicalScanningControl->addWidget(m_pLabel_RotationSpeed, 3, 1);
	pGridLayout_HelicalScanningControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 3, 2);
	pGridLayout_HelicalScanningControl->addWidget(m_pLineEdit_RPM, 3, 3);
	pGridLayout_HelicalScanningControl->addWidget(m_pLabel_RPM, 3, 4);
	pGridLayout_HelicalScanningControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 3, 5);
	pGridLayout_HelicalScanningControl->addWidget(m_pToggleButton_Rotate, 3, 6, 1, 2);

	m_pGroupBox_HelicalScanningControl->setLayout(pGridLayout_HelicalScanningControl);
	m_pVBoxLayout->addWidget(m_pGroupBox_HelicalScanningControl);
	m_pVBoxLayout->addStretch(1);

	// Connect signal and slot
	connect(m_pGroupBox_HelicalScanningControl, SIGNAL(toggled(bool)), this, SLOT(initializeHelicalScanning(bool)));

	connect(m_pLineEdit_PullbackSpeed, SIGNAL(textChanged(const QString &)), this, SLOT(setTargetSpeed(const QString &)));
	connect(m_pLineEdit_PullbackLength, SIGNAL(textChanged(const QString &)), this, SLOT(changePullbackLength(const QString &)));
	connect(m_pPushButton_Pullback, SIGNAL(clicked(bool)), this, SLOT(moveAbsolute()));
	connect(m_pPushButton_Home, SIGNAL(clicked(bool)), this, SLOT(home()));
	connect(m_pPushButton_PullbackStop, SIGNAL(clicked(bool)), this, SLOT(stop()));

	connect(m_pLineEdit_RPM, SIGNAL(textChanged(const QString &)), this, SLOT(changeRotaryRpm(const QString &)));
	connect(m_pToggleButton_Rotate, SIGNAL(toggled(bool)), this, SLOT(rotate(bool)));
}

void QDeviceControlTab::createFlimSystemControl()
{
	// FLIm System Control GroupBox
	m_pGroupBox_FlimControl = new QGroupBox;
	m_pGroupBox_FlimControl->setTitle("FLIm System Control  ");
	m_pGroupBox_FlimControl->setCheckable(true);
	m_pGroupBox_FlimControl->setChecked(false);
	m_pGroupBox_FlimControl->setStyleSheet("QGroupBox { padding-top: 18px; margin-top: -10px; } QGroupBox::title { subcontrol-origin: margin; left: 8px; top: 2px; }");

	QGridLayout *pGridLayout_FlimControl = new QGridLayout;
	pGridLayout_FlimControl->setSpacing(2);

    // Create widgets for FLIM laser control	
	m_pToggleButton_AsynchronizedPulsedLaser = new QPushButton(m_pGroupBox_FlimControl);
	m_pToggleButton_AsynchronizedPulsedLaser->setText("On");
	m_pToggleButton_AsynchronizedPulsedLaser->setFixedWidth(40);
	m_pToggleButton_AsynchronizedPulsedLaser->setCheckable(true);
	m_pToggleButton_AsynchronizedPulsedLaser->setDisabled(true);

	m_pLabel_AsynchronizedPulsedLaser = new QLabel("Asynchronized UV Pulsed Laser", m_pGroupBox_FlimControl);
	m_pLabel_AsynchronizedPulsedLaser->setBuddy(m_pToggleButton_AsynchronizedPulsedLaser);
	m_pLabel_AsynchronizedPulsedLaser->setDisabled(true);

	m_pToggleButton_SynchronizedPulsedLaser = new QPushButton(m_pGroupBox_FlimControl);
	m_pToggleButton_SynchronizedPulsedLaser->setText("On");
	m_pToggleButton_SynchronizedPulsedLaser->setFixedWidth(40);
	m_pToggleButton_SynchronizedPulsedLaser->setCheckable(true);
	m_pToggleButton_SynchronizedPulsedLaser->setDisabled(true);

	m_pLabel_SynchronizedPulsedLaser = new QLabel("Synchronized UV Pulsed Laser", m_pGroupBox_FlimControl);
	m_pLabel_SynchronizedPulsedLaser->setBuddy(m_pToggleButton_SynchronizedPulsedLaser);
	m_pLabel_SynchronizedPulsedLaser->setDisabled(true);
	
	// Create widgets for PMT gain control
	m_pLineEdit_PmtGainVoltage = new QLineEdit(m_pGroupBox_FlimControl);
	m_pLineEdit_PmtGainVoltage->setFixedWidth(40);
	m_pLineEdit_PmtGainVoltage->setText(QString::number(m_pConfig->pmtGainVoltage, 'f', 3));
	m_pLineEdit_PmtGainVoltage->setAlignment(Qt::AlignCenter);
	m_pLineEdit_PmtGainVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_PmtGainVoltage->setDisabled(true);

	m_pLabel_PmtGainControl = new QLabel("PMT Gain Voltage", m_pGroupBox_FlimControl);
	m_pLabel_PmtGainControl->setBuddy(m_pLineEdit_PmtGainVoltage);
	m_pLabel_PmtGainControl->setDisabled(true);

	m_pLabel_PmtGainVoltage = new QLabel("V  ", m_pGroupBox_FlimControl);
	m_pLabel_PmtGainVoltage->setBuddy(m_pLineEdit_PmtGainVoltage);
	m_pLabel_PmtGainVoltage->setDisabled(true);

	m_pToggleButton_PmtGainVoltage = new QPushButton(m_pGroupBox_FlimControl);
	m_pToggleButton_PmtGainVoltage->setText("On");	
	m_pToggleButton_PmtGainVoltage->setFixedWidth(40);
	m_pToggleButton_PmtGainVoltage->setCheckable(true);
	m_pToggleButton_PmtGainVoltage->setDisabled(true);
	
	// Create widgets for FLIM laser power control
	m_pSpinBox_FlimLaserPowerControl = new QSpinBox(m_pGroupBox_FlimControl);
	m_pSpinBox_FlimLaserPowerControl->setValue(0);
	m_pSpinBox_FlimLaserPowerControl->setRange(-10000, 10000);
	m_pSpinBox_FlimLaserPowerControl->setFixedWidth(15);
	m_pSpinBox_FlimLaserPowerControl->setDisabled(true);
	static int flim_laser_power_level = 0;

	m_pLabel_FlimLaserPowerControl = new QLabel("Laser Power Level Adjustment", m_pGroupBox_FlimControl);
	m_pLabel_FlimLaserPowerControl->setBuddy(m_pSpinBox_FlimLaserPowerControl);
	m_pLabel_FlimLaserPowerControl->setDisabled(true);

	// Create widgets for FLIm Calibration
	m_pPushButton_FlimCalibration = new QPushButton(m_pGroupBox_FlimControl);
	m_pPushButton_FlimCalibration->setText("FLIm Pulse View and Calibration...");
	m_pPushButton_FlimCalibration->setDisabled(true);

	// Set Layout
	QLabel *pLabel_Null = new QLabel(m_pGroupBox_FlimControl);
	pLabel_Null->setFixedWidth(50);
	
	pGridLayout_FlimControl->addWidget(pLabel_Null, 0, 0);
	pGridLayout_FlimControl->addWidget(m_pLabel_AsynchronizedPulsedLaser, 0, 1);
	pGridLayout_FlimControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 2, 1, 3);
	pGridLayout_FlimControl->addWidget(m_pToggleButton_AsynchronizedPulsedLaser, 0, 5, 1, 2);

	pGridLayout_FlimControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_FlimControl->addWidget(m_pLabel_SynchronizedPulsedLaser, 1, 1);
	pGridLayout_FlimControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 2, 1, 3);
	pGridLayout_FlimControl->addWidget(m_pToggleButton_SynchronizedPulsedLaser, 1, 5, 1, 2);
	
	pGridLayout_FlimControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 0);
	pGridLayout_FlimControl->addWidget(m_pLabel_PmtGainControl, 2, 1);
	pGridLayout_FlimControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 2);
	pGridLayout_FlimControl->addWidget(m_pLineEdit_PmtGainVoltage, 2, 3);
	pGridLayout_FlimControl->addWidget(m_pLabel_PmtGainVoltage, 2, 4);
	pGridLayout_FlimControl->addWidget(m_pToggleButton_PmtGainVoltage, 2, 5, 1, 2);
	
	pGridLayout_FlimControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 3, 0);
	pGridLayout_FlimControl->addWidget(m_pLabel_FlimLaserPowerControl, 3, 1);
	pGridLayout_FlimControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 3, 2, 1, 4);
	pGridLayout_FlimControl->addWidget(m_pSpinBox_FlimLaserPowerControl, 3, 6);

	pGridLayout_FlimControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 4, 0);
	pGridLayout_FlimControl->addWidget(m_pPushButton_FlimCalibration, 4, 1, 1, 6);
	
	m_pGroupBox_FlimControl->setLayout(pGridLayout_FlimControl);
	m_pVBoxLayout->addWidget(m_pGroupBox_FlimControl);
	
    // Connect signal and slot
	connect(m_pGroupBox_FlimControl, SIGNAL(toggled(bool)), this, SLOT(initializeFlimSystem(bool)));
	connect(m_pToggleButton_AsynchronizedPulsedLaser, SIGNAL(toggled(bool)), this, SLOT(startFlimAsynchronization(bool)));
	connect(m_pToggleButton_SynchronizedPulsedLaser, SIGNAL(toggled(bool)), this, SLOT(startFlimSynchronization(bool)));
	connect(m_pLineEdit_PmtGainVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changePmtGainVoltage(const QString &)));
    connect(m_pToggleButton_PmtGainVoltage, SIGNAL(toggled(bool)), this, SLOT(applyPmtGainVoltage(bool)));
	connect(m_pSpinBox_FlimLaserPowerControl, SIGNAL(valueChanged(int)), this, SLOT(adjustLaserPower(int)));
	connect(m_pPushButton_FlimCalibration, SIGNAL(clicked(bool)), this, SLOT(createFlimCalibDlg()));
}

void QDeviceControlTab::createAxsunOctSystemControl()
{
	// Axsun OCT System Control GroupBox
    m_pGroupBox_AxsunOctControl = new QGroupBox;
	m_pGroupBox_AxsunOctControl->setTitle("Axsun OCT System Control  ");
	m_pGroupBox_AxsunOctControl->setCheckable(true);
	m_pGroupBox_AxsunOctControl->setChecked(false);
	m_pGroupBox_AxsunOctControl->setStyleSheet("QGroupBox { padding-top: 18px; margin-top: -10px; } QGroupBox::title { subcontrol-origin: margin; left: 8px; top: 2px; }");

    QGridLayout *pGridLayout_AxsunOctControl = new QGridLayout;
	pGridLayout_AxsunOctControl->setSpacing(2);

	// Create widgets for Axsun OCT control
    m_pToggleButton_LightSource = new QPushButton(m_pGroupBox_AxsunOctControl);
    m_pToggleButton_LightSource->setCheckable(true);
    m_pToggleButton_LightSource->setFixedWidth(40);
    m_pToggleButton_LightSource->setText("On");
    m_pToggleButton_LightSource->setDisabled(true);

	m_pLabel_LightSource = new QLabel("Swept-Source OCT Laser", m_pGroupBox_AxsunOctControl);
	m_pLabel_LightSource->setBuddy(m_pToggleButton_LightSource);
	m_pLabel_LightSource->setDisabled(true);

	m_pToggleButton_LiveImaging = new QPushButton(m_pGroupBox_AxsunOctControl);
	m_pToggleButton_LiveImaging->setCheckable(true);
	m_pToggleButton_LiveImaging->setFixedWidth(40);
	m_pToggleButton_LiveImaging->setText("On");
	m_pToggleButton_LiveImaging->setDisabled(true);

	m_pLabel_LiveImaging = new QLabel("Axsun OCT Live Imaging Mode", m_pGroupBox_AxsunOctControl);
	m_pLabel_LiveImaging->setBuddy(m_pToggleButton_LiveImaging);
	m_pLabel_LiveImaging->setDisabled(true);
	
    m_pSpinBox_VDLLength = new QMySpinBox(m_pGroupBox_AxsunOctControl);
    m_pSpinBox_VDLLength->setFixedWidth(55);
    m_pSpinBox_VDLLength->setRange(0.00, 15.00);
    m_pSpinBox_VDLLength->setSingleStep(0.05);
    m_pSpinBox_VDLLength->setValue(m_pConfig->axsunVDLLength);
    m_pSpinBox_VDLLength->setDecimals(2);
    m_pSpinBox_VDLLength->setAlignment(Qt::AlignCenter);
    m_pSpinBox_VDLLength->setDisabled(true);

    m_pLabel_VDLLength = new QLabel("VDL Position Adjustment (mm)", m_pGroupBox_AxsunOctControl);
    m_pLabel_VDLLength->setBuddy(m_pSpinBox_VDLLength);
    m_pLabel_VDLLength->setDisabled(true);

	m_pPushButton_VDLHome = new QPushButton(m_pGroupBox_AxsunOctControl);
	m_pPushButton_VDLHome->setFixedWidth(40);
	m_pPushButton_VDLHome->setText("Home");
	m_pPushButton_VDLHome->setDisabled(true);
	
	// Set Layout
	QLabel *pLabel_Null = new QLabel(m_pGroupBox_AxsunOctControl);
	pLabel_Null->setFixedWidth(50);

	pGridLayout_AxsunOctControl->addWidget(pLabel_Null, 0, 0);
	pGridLayout_AxsunOctControl->addWidget(m_pLabel_LightSource, 0, 1, 1, 2);
	pGridLayout_AxsunOctControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 3);
	pGridLayout_AxsunOctControl->addWidget(m_pToggleButton_LightSource, 0, 4);

	pGridLayout_AxsunOctControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_AxsunOctControl->addWidget(m_pLabel_LiveImaging, 1, 1, 1, 2);
	pGridLayout_AxsunOctControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 3);
	pGridLayout_AxsunOctControl->addWidget(m_pToggleButton_LiveImaging, 1, 4);
	
	pGridLayout_AxsunOctControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 0);
	pGridLayout_AxsunOctControl->addWidget(m_pLabel_VDLLength, 2, 1);
	pGridLayout_AxsunOctControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 2);
	pGridLayout_AxsunOctControl->addWidget(m_pSpinBox_VDLLength, 2, 3);
	pGridLayout_AxsunOctControl->addWidget(m_pPushButton_VDLHome, 2, 4);
	
	m_pGroupBox_AxsunOctControl->setLayout(pGridLayout_AxsunOctControl);
	m_pVBoxLayout->addWidget(m_pGroupBox_AxsunOctControl);
	
    // Connect signal and slot
	connect(m_pGroupBox_AxsunOctControl, SIGNAL(toggled(bool)), this, SLOT(connectAxsunControl(bool)));
	connect(m_pToggleButton_LightSource, SIGNAL(toggled(bool)), this, SLOT(setLightSource(bool)));
	connect(m_pToggleButton_LiveImaging, SIGNAL(toggled(bool)), this, SLOT(setLiveImaging(bool)));
	connect(m_pSpinBox_VDLLength, SIGNAL(valueChanged(double)), this, SLOT(setVDLLength(double)));
	connect(m_pPushButton_VDLHome, SIGNAL(clicked(bool)), this, SLOT(setVDLHome()));
	connect(this, SIGNAL(transferAxsunArray(int)), m_pStreamTab->getOperationTab()->getProgressBar(), SLOT(setValue(int)));
}


void QDeviceControlTab::initializeHelicalScanning(bool toggled)
{
	if (toggled)
	{
		// Faulhaber Pullback Motor Connection Check
		//if (connectPullbackMotor(true))
		//{
		//	// Set enable true for pullback motor control widgets
		//	m_pLabel_PullbackSpeed->setEnabled(true);
		//	m_pLineEdit_PullbackSpeed->setEnabled(true);
		//	m_pLabel_PullbackSpeedUnit->setEnabled(true);
		//	m_pLabel_PullbackLength->setEnabled(true);
		//	m_pLineEdit_PullbackLength->setEnabled(true);
		//	m_pLabel_PullbackLengthUnit->setEnabled(true);
		//	m_pPushButton_Pullback->setEnabled(true);
		//	m_pPushButton_Home->setEnabled(true);
		//	m_pPushButton_PullbackStop->setEnabled(true);
		//	m_pPushButton_Pullback->setStyleSheet("QPushButton { background-color:#ff0000; }");
		//}
		///else
		///{
		///	m_pGroupBox_HelicalScanningControl->setChecked(false);
		///	return;
		///}

		// Faulhaber Rotary Motor Connection Check
		//if (connectRotaryMotor(true))
		//{
		//	// Set enable true for Faulhaber motor control widgets
		//	m_pLabel_RotationSpeed->setEnabled(true);
		//	m_pLineEdit_RPM->setEnabled(true);
		//	m_pLabel_RPM->setEnabled(true);
		//	m_pToggleButton_Rotate->setEnabled(true);
		//	m_pToggleButton_Rotate->setStyleSheet("QPushButton { background-color:#ff0000; }");
		//}
		///else
		///{
		///	m_pGroupBox_HelicalScanningControl->setChecked(false);
		///	return;
		///}
	}
	else
	{
		// Set enable false for Faulhaber pullback motor control widgets
		m_pLabel_PullbackSpeed->setEnabled(false);
		m_pLineEdit_PullbackSpeed->setEnabled(false);
		m_pLabel_PullbackSpeedUnit->setEnabled(false);
		m_pLabel_PullbackLength->setEnabled(false);
		m_pLineEdit_PullbackLength->setEnabled(false);
		m_pLabel_PullbackLengthUnit->setEnabled(false);
		m_pPushButton_Pullback->setEnabled(false);
		m_pPushButton_Home->setEnabled(false);
		m_pPushButton_PullbackStop->setEnabled(false);
		m_pPushButton_Pullback->setStyleSheet("QPushButton { background-color:#353535; }");

		// Set enable false for Faulhaber rotary motor control widgets
		if (m_pToggleButton_Rotate->isChecked()) m_pToggleButton_Rotate->setChecked(false);
		m_pToggleButton_Rotate->setEnabled(false);
		m_pToggleButton_Rotate->setStyleSheet("QPushButton { background-color:#353535; }");
		m_pLabel_RotationSpeed->setEnabled(false);
		m_pLineEdit_RPM->setEnabled(false);
		m_pLabel_RPM->setEnabled(false);

		// Disconnect devices
		//connectPullbackMotor(false);
		connectRotaryMotor(false);
	}
}

bool QDeviceControlTab::connectPullbackMotor(bool toggled)
{
	if (toggled)
	{
		// Create Faulhaber pullback motor control objects
		if (!m_pPullbackMotor)
		{
			m_pPullbackMotor = new PullbackMotor;
			m_pPullbackMotor->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				qmsg.replace('\n', ' ');
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
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

void QDeviceControlTab::moveAbsolute()
{
	// Set widgets
	m_pLabel_PullbackSpeed->setDisabled(true);
	m_pLabel_PullbackSpeedUnit->setDisabled(true);
	m_pLineEdit_PullbackSpeed->setDisabled(true);

	m_pLabel_PullbackLength->setDisabled(true);
	m_pLabel_PullbackLengthUnit->setDisabled(true);
	m_pLineEdit_PullbackLength->setDisabled(true);

	m_pPushButton_Home->setDisabled(true);
	
	m_pPushButton_Pullback->setStyleSheet("QPushButton { background-color:#00ff00; }");

	// Pullback
	m_pPullbackMotor->RotateMotor(-int(m_pConfig->pullbackSpeed * GEAR_RATITO));

	// Pullback end condition
	double duration = (double)(m_pConfig->pullbackLength) / (double)(m_pConfig->pullbackSpeed);
	std::thread stop([&, duration]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(int(1000 * 2)));// duration)));
		this->stop();
	});
	stop.detach();
}

void QDeviceControlTab::setTargetSpeed(const QString & str)
{
	m_pConfig->pullbackSpeed = str.toInt();
}

void QDeviceControlTab::changePullbackLength(const QString &str)
{
	m_pConfig->pullbackLength = str.toInt();
}

void QDeviceControlTab::home()
{
	m_pPullbackMotor->RotateMotor(int(20 * GEAR_RATITO));
}

void QDeviceControlTab::stop()
{
	// Stop pullback motor
	m_pPullbackMotor->StopMotor();

	// Stop rotary motor
	if (m_pToggleButton_Rotate->isChecked())
	{
		m_pToggleButton_Rotate->setChecked(false);

		m_pGroupBox_HelicalScanningControl->setChecked(false);
		m_pGroupBox_HelicalScanningControl->setChecked(true);
	}

	// Set widgets
	m_pLabel_PullbackSpeed->setEnabled(true);
	m_pLabel_PullbackSpeedUnit->setEnabled(true);
	m_pLineEdit_PullbackSpeed->setEnabled(true);

	m_pLabel_PullbackLength->setEnabled(true);
	m_pLabel_PullbackLengthUnit->setEnabled(true);
	m_pLineEdit_PullbackLength->setEnabled(true);

	m_pPushButton_Home->setEnabled(true);

	m_pPushButton_Pullback->setStyleSheet("QPushButton { background-color:#ff0000; }");
}

bool QDeviceControlTab::connectRotaryMotor(bool toggled)
{
	if (toggled)
	{
		// Create Faulhaber rotary motor control objects
		if (!m_pRotaryMotor)
		{
			m_pRotaryMotor = new RotaryMotor;
			m_pRotaryMotor->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				qmsg.replace('\n', ' ');
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
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

void QDeviceControlTab::rotate(bool toggled)
{
	m_pRotaryMotor->RotateMotor(toggled ? m_pConfig->rotaryRpm : 0);
	
	m_pLineEdit_RPM->setDisabled(toggled);
	m_pLabel_RPM->setDisabled(toggled);

	m_pToggleButton_Rotate->setStyleSheet(toggled ? "QPushButton { background-color:#00ff00; }" : "QPushButton { background-color:#ff0000; }");
	m_pToggleButton_Rotate->setText(toggled ? "Stop" : "Rotate");
}

void QDeviceControlTab::changeRotaryRpm(const QString &str)
{
	m_pConfig->rotaryRpm = str.toInt();
}

void QDeviceControlTab::setHelicalScanningControl(bool toggled)
{
	// Set Helical Scanning Control
	m_pGroupBox_HelicalScanningControl->setChecked(toggled);
}


void QDeviceControlTab::initializeFlimSystem(bool toggled)
{
    if (toggled)
    {
		// DAQ Board Connection Check
		if (isDaqBoardConnected())
		{
			// Set enabled true for FLIm control widgets
			if (!m_pToggleButton_SynchronizedPulsedLaser->isChecked())
			{
				m_pLabel_AsynchronizedPulsedLaser->setEnabled(true);
				m_pToggleButton_AsynchronizedPulsedLaser->setEnabled(true);
			}
			if (!m_pToggleButton_AsynchronizedPulsedLaser->isChecked())
			{
				m_pLabel_SynchronizedPulsedLaser->setEnabled(true);
				m_pToggleButton_SynchronizedPulsedLaser->setEnabled(true);
			}
			m_pLabel_PmtGainControl->setEnabled(true);
			m_pLineEdit_PmtGainVoltage->setEnabled(true);
			m_pLabel_PmtGainVoltage->setEnabled(true);
			m_pToggleButton_PmtGainVoltage->setEnabled(true); 

			if (!m_pStreamTab->getOperationTab()->m_pAcquisitionState)
			{
				m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");
				m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");
				m_pToggleButton_PmtGainVoltage->setStyleSheet("QPushButton { background-color:#ff0000; }");
			}
		}
		else
		{
			m_pGroupBox_FlimControl->setChecked(false);
			return;
		}

		// FLIm Laser Connection Check
		if (connectFlimLaser(true))
		{
			// Set enabled true for FLIM laser power control widgets
			m_pLabel_FlimLaserPowerControl->setEnabled(true);
			m_pSpinBox_FlimLaserPowerControl->setEnabled(true);
		}
		else
		{
			m_pGroupBox_FlimControl->setChecked(false);
			return;
		}

		// Digitizer Connection Check
		if (m_pStreamTab->getOperationTab()->getDataAcq()->getDigitizer()->is_initialized())
		{
			// Set enabled true for FLIm pulse view and calibration button
			m_pPushButton_FlimCalibration->setEnabled(true);
		}
		///else
		///{
		///	m_pGroupBox_FlimControl->setChecked(false);
		///	return;
		///}
    }
    else
    {
		// If acquisition is processing...
		if (m_pStreamTab->getOperationTab()->m_pAcquisitionState)
		{
			QMessageBox MsgBox;
			MsgBox.setWindowTitle("Warning");
			MsgBox.setIcon(QMessageBox::Warning);
			MsgBox.setText("Re-turning the FLIm system on does not guarantee the synchronized operation once you turn off the laser.\nWould you like to turn off the FLIm system?");
			MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			MsgBox.setDefaultButton(QMessageBox::No);

			int resp = MsgBox.exec();
			switch (resp)
			{
			case QMessageBox::Yes:
				m_pStreamTab->getOperationTab()->setAcquisitionButton(false);
				break;
			case QMessageBox::No:
				m_pGroupBox_FlimControl->setChecked(true);
				return;
			default:
				m_pGroupBox_FlimControl->setChecked(true);
				return;
			}
		}
		
		// Set buttons
		if (m_pToggleButton_AsynchronizedPulsedLaser->isChecked()) m_pToggleButton_AsynchronizedPulsedLaser->setChecked(false);
		if (m_pToggleButton_SynchronizedPulsedLaser->isChecked()) m_pToggleButton_SynchronizedPulsedLaser->setChecked(false);
		if (m_pToggleButton_PmtGainVoltage->isChecked()) m_pToggleButton_PmtGainVoltage->setChecked(false);
		m_pToggleButton_SynchronizedPulsedLaser->setText("On");

		m_pLabel_AsynchronizedPulsedLaser->setEnabled(false);
		m_pToggleButton_AsynchronizedPulsedLaser->setEnabled(false);
		m_pLabel_SynchronizedPulsedLaser->setEnabled(false);
		m_pToggleButton_SynchronizedPulsedLaser->setEnabled(false);
		m_pLabel_PmtGainControl->setEnabled(false);
		m_pLineEdit_PmtGainVoltage->setEnabled(false);
		m_pLabel_PmtGainVoltage->setEnabled(false);
		m_pToggleButton_PmtGainVoltage->setEnabled(false);
		m_pLabel_FlimLaserPowerControl->setEnabled(false);
		m_pSpinBox_FlimLaserPowerControl->setEnabled(false);
		m_pPushButton_FlimCalibration->setEnabled(false);

		m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#353535; }");
		m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#353535; }");
		m_pToggleButton_PmtGainVoltage->setStyleSheet("QPushButton { background-color:#353535; }");

		// Disconnect devices
		connectFlimLaser(false);		
    }
}

bool QDeviceControlTab::isDaqBoardConnected()
{
#ifdef NI_ENABLE
	if (!m_pPmtGainControl)
	{
		// Create temporary PMT gain control objects
		PmtGainControl *pTempGain = new PmtGainControl;
		pTempGain->SendStatusMessage += [&](const char* msg, bool is_error) {
			QString qmsg = QString::fromUtf8(msg);
			emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
		};

		pTempGain->voltage = 0;
		if (!pTempGain->initialize())
		{
			pTempGain->SendStatusMessage("DAQ Board is not connected!", true);
			delete pTempGain;
			return false;
		}

		pTempGain->stop();
		delete pTempGain;
	}

	return true;
#else
	return false;
#endif
}

void QDeviceControlTab::startFlimAsynchronization(bool toggled)
{
#ifdef NI_ENABLE
	if (toggled)
	{
		// Set text
		m_pToggleButton_AsynchronizedPulsedLaser->setText("Off");
		m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#00ff00; }");

		// Create FLIm laser async control objects
		if (!m_pFlimFreqDivider)
		{
			m_pFlimFreqDivider = new FreqDivider;
			m_pFlimFreqDivider->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}
		m_pFlimFreqDivider->sourceTerminal = "100kHzTimebase";
		m_pFlimFreqDivider->counterChannel = NI_FLIM_TRIG_CHANNEL;
		m_pFlimFreqDivider->slow = 4;

		// Initializing
		if (!m_pFlimFreqDivider->initialize())
		{
			m_pToggleButton_AsynchronizedPulsedLaser->setChecked(false);
			return;
		}

		// Generate FLIm laser
		m_pFlimFreqDivider->start();

		// Set widgets
		m_pLabel_SynchronizedPulsedLaser->setEnabled(false);
		m_pToggleButton_SynchronizedPulsedLaser->setEnabled(false);
		m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#353535; }");
	}
	else
	{
		// Delete FLIm laser async control objects
		if (m_pFlimFreqDivider)
		{
			m_pFlimFreqDivider->stop();
			delete m_pFlimFreqDivider;
			m_pFlimFreqDivider = nullptr;
		}

		// Set text
		m_pToggleButton_AsynchronizedPulsedLaser->setText("On");
		m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");

		// Set widgets
		m_pLabel_SynchronizedPulsedLaser->setEnabled(true);
		m_pToggleButton_SynchronizedPulsedLaser->setEnabled(true);
		m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");
	}
#else
	(void)toggled;
#endif
}

void QDeviceControlTab::startFlimSynchronization(bool toggled)
{
#ifdef NI_ENABLE
	if (toggled)
	{
		// Set text
		m_pToggleButton_SynchronizedPulsedLaser->setText("Off");
		m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#00ff00; }");

		// Create FLIm laser sync control objects
		if (!m_pFlimFreqDivider)
		{
			m_pFlimFreqDivider = new FreqDivider;
			m_pFlimFreqDivider->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}
		m_pFlimFreqDivider->sourceTerminal = NI_FLIM_TRIG_SOURCE;
		m_pFlimFreqDivider->counterChannel = NI_FLIM_TRIG_CHANNEL;
		m_pFlimFreqDivider->slow = 4;

		// Create Axsun OCT sync control objects
		if (!m_pAxsunFreqDivider)
		{
			m_pAxsunFreqDivider = new FreqDivider;
			m_pAxsunFreqDivider->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}
		m_pAxsunFreqDivider->sourceTerminal = NI_AXSUN_TRIG_SOURCE;
		m_pAxsunFreqDivider->counterChannel = NI_AXSUN_TRIG_CHANNEL;
		m_pAxsunFreqDivider->slow = 1024;

		// Initializing
		if (!m_pFlimFreqDivider->initialize() || !m_pAxsunFreqDivider->initialize())
		{
			m_pToggleButton_SynchronizedPulsedLaser->setChecked(false);
			return;
		}

		// Generate FLIm laser & Axsun OCT sync
		m_pFlimFreqDivider->start();
		m_pAxsunFreqDivider->start();

		// Set widgets
		m_pLabel_AsynchronizedPulsedLaser->setEnabled(false);
		m_pToggleButton_AsynchronizedPulsedLaser->setEnabled(false);
		m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#353535; }");
	}
	else
	{
		// If acquisition is processing...
		if (m_pStreamTab->getOperationTab()->m_pAcquisitionState)
		{
			QMessageBox MsgBox;
			MsgBox.setWindowTitle("Warning");
			MsgBox.setIcon(QMessageBox::Warning);
			MsgBox.setText("Re-turning the laser on does not guarantee the synchronized operation once you turn off the laser.\nWould you like to turn off the laser?");
			MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			MsgBox.setDefaultButton(QMessageBox::No);

			int resp = MsgBox.exec();
			switch (resp)
			{
				case QMessageBox::Yes:
					m_pStreamTab->getOperationTab()->setAcquisitionButton(false);
					break;
				case QMessageBox::No:
					m_pToggleButton_SynchronizedPulsedLaser->setChecked(true);
					return;
				default:
					m_pToggleButton_SynchronizedPulsedLaser->setChecked(true);
					return;
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

			// Set text
			m_pToggleButton_SynchronizedPulsedLaser->setText("On");
			m_pToggleButton_SynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");

			// Set widgets
			m_pLabel_AsynchronizedPulsedLaser->setEnabled(true);
			m_pToggleButton_AsynchronizedPulsedLaser->setEnabled(true);
			m_pToggleButton_AsynchronizedPulsedLaser->setStyleSheet("QPushButton { background-color:#ff0000; }");
		}
	}
#else
	(void)toggled;
#endif
}

void QDeviceControlTab::applyPmtGainVoltage(bool toggled)
{
#ifdef NI_ENABLE
	// Set text
	m_pToggleButton_PmtGainVoltage->setText(toggled ? "Off" : "On");
	m_pToggleButton_PmtGainVoltage->setStyleSheet(toggled ? "QPushButton { background-color:#00ff00; }" : "QPushButton { background-color:#ff0000; }");
	
	if (toggled)
	{
		// Set enabled false for PMT gain control widgets
		m_pLineEdit_PmtGainVoltage->setEnabled(false);
		m_pLabel_PmtGainVoltage->setEnabled(false);

		// Create PMT gain control objects
		if (!m_pPmtGainControl)
		{
			m_pPmtGainControl = new PmtGainControl;
			m_pPmtGainControl->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}

		m_pPmtGainControl->voltage = m_pLineEdit_PmtGainVoltage->text().toDouble();
		if (m_pPmtGainControl->voltage > 1.0)
		{
			m_pPmtGainControl->SendStatusMessage(">1.0V Gain cannot be assigned!", true);
			m_pToggleButton_PmtGainVoltage->setChecked(false);
			return;
		}

		// Initializing
		if (!m_pPmtGainControl->initialize())
		{
			m_pToggleButton_PmtGainVoltage->setChecked(false);
			return;
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
		m_pLineEdit_PmtGainVoltage->setEnabled(true);
		m_pLabel_PmtGainVoltage->setEnabled(true);
	}	
#else
	(void)toggled;
#endif
}

void QDeviceControlTab::changePmtGainVoltage(const QString & str)
{
	m_pConfig->pmtGainVoltage = str.toFloat();
}

bool QDeviceControlTab::connectFlimLaser(bool state)
{
	if (state)
	{
		// Create FLIM laser power control objects
		if (!m_pElforlightLaser)
		{
			m_pElforlightLaser = new ElforlightLaser;
			m_pElforlightLaser->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				qmsg.replace('\n', ' ');
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
			m_pElforlightLaser->UpdateState += [&](double* state) {
				m_pStreamTab->getMainWnd()->updateFlimLaserState(state);
			};
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

void QDeviceControlTab::adjustLaserPower(int level)
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
		m_pElforlightLaser->IncreasePower();
		flim_laser_power_level++;
	}
	else
	{
		m_pElforlightLaser->DecreasePower();
		flim_laser_power_level--;
	}
}

void QDeviceControlTab::sendLaserCommand(char* command)
{
	if (m_pElforlightLaser) 
		//if (m_pElforlightLaser->isLaserEnabled)
		m_pElforlightLaser->SendCommand(command);
}

void QDeviceControlTab::createFlimCalibDlg()
{
    if (m_pFlimCalibDlg == nullptr)
    {
        m_pFlimCalibDlg = new FlimCalibDlg(this);
        connect(m_pFlimCalibDlg, SIGNAL(finished(int)), this, SLOT(deleteFlimCalibDlg()));
		m_pFlimCalibDlg->SendStatusMessage += [&](const char* msg) {
			QString qmsg = QString::fromUtf8(msg);
			emit m_pStreamTab->sendStatusMessage(qmsg, false);
		};
        m_pFlimCalibDlg->show();
///        emit m_pFlimCalibDlg->plotRoiPulse(m_pFLIm, m_pSlider_SelectAline->value() / 4);
    }
    m_pFlimCalibDlg->raise();
    m_pFlimCalibDlg->activateWindow();
}

void QDeviceControlTab::deleteFlimCalibDlg()
{
///    m_pFlimCalibDlg->showWindow(false);
///    m_pFlimCalibDlg->showMeanDelay(false);
///    m_pFlimCalibDlg->showMask(false);

    m_pFlimCalibDlg->deleteLater();
    m_pFlimCalibDlg = nullptr;
}

void QDeviceControlTab::setFlimControl(bool toggled)
{
	// Set FLIm Control
	if (toggled)
	{
		if (m_pGroupBox_FlimControl->isChecked()) m_pGroupBox_FlimControl->setChecked(false);
		m_pGroupBox_FlimControl->setChecked(toggled);
		if (m_pElforlightLaser)
		{
			m_pToggleButton_SynchronizedPulsedLaser->setChecked(toggled);
			m_pToggleButton_PmtGainVoltage->setChecked(toggled);
		}
	}
	else
		m_pGroupBox_FlimControl->setChecked(toggled);
}


void QDeviceControlTab::connectAxsunControl(bool toggled)
{
	if (toggled)
	{
		// Create Axsun OCT control objects
		if (!m_pAxsunControl)
		{
			m_pAxsunControl = new AxsunControl;
			m_pAxsunControl->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
				if (is_error)
				{
					m_pStreamTab->getOperationTab()->getProgressBar()->setFormat("");
					m_pStreamTab->getOperationTab()->getProgressBar()->setValue(0);
				}
			};
			m_pAxsunControl->DidTransferArray += [&](int i) {
				emit transferAxsunArray(i);
			};		
			
			// Initialize the Axsun OCT control
			if (!(m_pAxsunControl->initialize()))
			{
				m_pGroupBox_AxsunOctControl->setChecked(false);
				return;
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
				if (m_pStreamTab->getOperationTab()->getAcquisitionButton()->isChecked())
					setVDLWidgets(true);
			});
			vdl_length.detach();

			// Default Contrast Range
			adjustDecibelRange();
		}
				
		// Set enabled true for Axsun OCT control widgets
		m_pLabel_LightSource->setEnabled(true);
		m_pLabel_LiveImaging->setEnabled(true);
		m_pToggleButton_LightSource->setEnabled(true);
		m_pToggleButton_LiveImaging->setEnabled(true);

		if (!m_pStreamTab->getOperationTab()->m_pAcquisitionState)
		{
			m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#ff0000; }");
			m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#ff0000; }");
		}
		
		m_pStreamTab->getVisTab()->setOctDecibelContrastWidgets(true);
	}
	else
	{
		// If acquisition is processing...
		if (m_pStreamTab->getOperationTab()->m_pAcquisitionState)
		{
			QMessageBox MsgBox;
			MsgBox.setWindowTitle("Warning");
			MsgBox.setIcon(QMessageBox::Warning);
			MsgBox.setText("Re-turning the Axsun OCT system on does not guarantee the synchronized operation once you turn off the laser.\nWould you like to turn off the Axsun OCT system?");
			MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			MsgBox.setDefaultButton(QMessageBox::No);

			int resp = MsgBox.exec();
			switch (resp)
			{
			case QMessageBox::Yes:
				m_pStreamTab->getOperationTab()->setAcquisitionButton(false);
				break;
			case QMessageBox::No:
				m_pGroupBox_AxsunOctControl->setChecked(true);
				return;
			default:
				m_pGroupBox_AxsunOctControl->setChecked(true);
				return;
			}
		}

		// Set buttons
		if (m_pToggleButton_LightSource->isChecked()) m_pToggleButton_LightSource->setChecked(false);
		if (m_pToggleButton_LiveImaging->isChecked()) m_pToggleButton_LiveImaging->setChecked(false);
		m_pToggleButton_LightSource->setText("On");
		m_pToggleButton_LiveImaging->setText("On");

		// Set disabled true for Axsun OCT control widgets
		m_pLabel_LightSource->setDisabled(true);
		m_pLabel_LiveImaging->setDisabled(true);
		m_pToggleButton_LightSource->setDisabled(true);
		m_pToggleButton_LiveImaging->setDisabled(true);
		m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#353535; }");
		m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#353535; }");
		
		//m_pLabel_VDLLength->setDisabled(true);
		//m_pSpinBox_VDLLength->setDisabled(true);
		//m_pPushButton_VDLHome->setDisabled(true);

		m_pStreamTab->getVisTab()->setOctDecibelContrastWidgets(false);

		if (m_pAxsunControl)
		{
			// Delete Axsun OCT control objects
			delete m_pAxsunControl;
			m_pAxsunControl = nullptr;
		}
	}
}

void QDeviceControlTab::setLightSource(bool toggled)
{
	if (m_pAxsunControl)
	{
		if (toggled)
		{
			// Set text
			m_pToggleButton_LightSource->setText("Off");
			m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#00ff00; }");

			// Start Axsun light source operation
			m_pAxsunControl->setLaserEmission(true);
		}
		else
		{
			// If acquisition is processing...
			if (m_pStreamTab->getOperationTab()->m_pAcquisitionState)
			{
				QMessageBox MsgBox;
				MsgBox.setWindowTitle("Warning");
				MsgBox.setIcon(QMessageBox::Warning);
				MsgBox.setText("Re-turning the laser on does not guarantee the synchronized operation once you turn off the laser.\nWould you like to turn off the laser?");
				MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
				MsgBox.setDefaultButton(QMessageBox::No);

				int resp = MsgBox.exec();
				switch (resp)
				{
				case QMessageBox::Yes:
					m_pStreamTab->getOperationTab()->setAcquisitionButton(false);
					break;
				case QMessageBox::No:
					m_pToggleButton_LightSource->setChecked(true);
					return;
				default:
					m_pToggleButton_LightSource->setChecked(true);
					return;
				}
			}
			else
			{
				// Stop Axsun light source operation
				if (m_pAxsunControl) m_pAxsunControl->setLaserEmission(false);

				// Set text
				m_pToggleButton_LightSource->setText("On");
				m_pToggleButton_LightSource->setStyleSheet("QPushButton { background-color:#ff0000; }");
			}
		}
	}
}

void QDeviceControlTab::setLiveImaging(bool toggled)
{
	if (m_pAxsunControl)
	{
		if (toggled)
		{
			// Set text and widgets
			m_pToggleButton_LiveImaging->setText("Off");
			m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#00ff00; }");

			// Start Axsun live imaging operation
			m_pAxsunControl->setLiveImagingMode(true);
		}
		else
		{
			// If acquisition is processing...
			if (m_pStreamTab->getOperationTab()->m_pAcquisitionState)
			{
				QMessageBox MsgBox;
				MsgBox.setWindowTitle("Warning");
				MsgBox.setIcon(QMessageBox::Warning);
				MsgBox.setText("Re-turning the live imaging mode on does not guarantee the synchronized operation once you turn off the live imaging mode.\nWould you like to turn off the live imaging mode?");
				MsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
				MsgBox.setDefaultButton(QMessageBox::No);

				int resp = MsgBox.exec();
				switch (resp)
				{
				case QMessageBox::Yes:
					m_pStreamTab->getOperationTab()->setAcquisitionButton(false);
					break;
				case QMessageBox::No:
					m_pToggleButton_LiveImaging->setChecked(true);
					return;
				default:
					m_pToggleButton_LiveImaging->setChecked(true);
					return;
				}
			}
			else
			{
				// Stop Axsun live imaging operation
				if (m_pAxsunControl) m_pAxsunControl->setLiveImagingMode(false);

				// Set text and widgets
				m_pToggleButton_LiveImaging->setText("On");
				m_pToggleButton_LiveImaging->setStyleSheet("QPushButton { background-color:#ff0000; }");
			}
		}
	}
}

void QDeviceControlTab::adjustDecibelRange()
{
	if (m_pAxsunControl)
		m_pAxsunControl->setDecibelRange(m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max);
}

void QDeviceControlTab::setClockDelay(double)
{
	if (m_pAxsunControl)
		m_pAxsunControl->setClockDelay(CLOCK_DELAY);
}

void QDeviceControlTab::setVDLLength(double length)
{
	if (m_pAxsunControl)
	{
		m_pConfig->axsunVDLLength = length;
		m_pAxsunControl->setVDLLength(length);
	}
}

void QDeviceControlTab::setVDLHome()
{
	if (m_pAxsunControl)
	{
		m_pAxsunControl->setVDLHome();
		m_pSpinBox_VDLLength->setValue(0.0);
	}
}

void QDeviceControlTab::setVDLWidgets(bool enabled)
{
	m_pLabel_VDLLength->setEnabled(enabled);
	m_pSpinBox_VDLLength->setEnabled(enabled);
	m_pPushButton_VDLHome->setEnabled(enabled);
}

void QDeviceControlTab::setAxsunControl(bool toggled)
{
	// Set Axsun OCT Control
	if (toggled)
	{
		if (m_pGroupBox_AxsunOctControl->isChecked()) m_pGroupBox_AxsunOctControl->setChecked(false);
		m_pGroupBox_AxsunOctControl->setChecked(toggled);
		if (m_pAxsunControl)
		{
			m_pToggleButton_LightSource->setChecked(toggled);
			m_pToggleButton_LiveImaging->setChecked(toggled);
		}
	}
	else
		m_pGroupBox_AxsunOctControl->setChecked(toggled);
}

void QDeviceControlTab::requestOctStatus()
{
	if (m_pAxsunControl) m_pAxsunControl->getDeviceState();
}