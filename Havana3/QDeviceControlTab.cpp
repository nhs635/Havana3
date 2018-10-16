
#include "QDeviceControlTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QOperationTab.h>

#include <Havana3/Dialog/FlimCalibDlg.h>
#include <Havana3/Viewer/QImageView.h>

#ifndef NI_ENABLE
#include <DeviceControl/ArduinoMCU/ArduinoMCU.h>
#else
#include <DeviceControl/FreqDivider/FreqDivider.h>
#include <DeviceControl/PmtGainControl/PmtGainControl.h>
#endif
#include <DeviceControl/ElforlightLaser/ElforlightLaser.h>
#include <DeviceControl/AxsunControl/AxsunControl.h>
#include <DeviceControl/ZaberStage/ZaberStage.h>
#include <DeviceControl/FaulhaberMotor/FaulhaberMotor.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/AxsunCapture/AxsunCapture.h>

#include <Common/Array.h>

#include <iostream>
#include <thread>


QDeviceControlTab::QDeviceControlTab(QWidget *parent) :
    QDialog(parent), 
#ifndef NI_ENABLE
	m_pArduinoMCU(nullptr), 
#else
	m_pFlimFreqDivider(nullptr), m_pAxsunFreqDivider(nullptr), m_pPmtGainControl(nullptr),
#endif	
	m_pElforlightLaser(nullptr), m_pFlimCalibDlg(nullptr),
    m_pAxsunControl(nullptr), m_pZaberStage(nullptr),
	m_pFaulhaberMotor(nullptr)
{
	// Set main window objects
    m_pStreamTab = dynamic_cast<QStreamTab*>(parent);
    m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;

    // Create layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(0);

	m_pVBoxLayout_FlimControl = new QVBoxLayout;
	m_pVBoxLayout_FlimControl->setSpacing(3);
    m_pGroupBox_FlimControl = new QGroupBox;

    createFlimSynchronizationControl();
    createPmtGainControl();
	createFlimLaserPowerControl();
    createFLimCalibControl();
    createAxsunOctControl();
    createZaberStageControl();
    createFaulhaberMotorControl();
	
    // Set layout
    setLayout(m_pVBoxLayout);
}

QDeviceControlTab::~QDeviceControlTab()
{
	setControlsStatus();
}

void QDeviceControlTab::closeEvent(QCloseEvent* e)
{
	e->accept();
}


void QDeviceControlTab::setControlsStatus()
{
	if (m_pFlimCalibDlg) m_pFlimCalibDlg->close();

	if (m_pCheckBox_FlimControl->isChecked()) m_pCheckBox_FlimControl->setChecked(false);
	if (m_pCheckBox_FlimLaserPowerControl->isChecked()) m_pCheckBox_FlimLaserPowerControl->setChecked(false);
	if (m_pCheckBox_AxsunOctControl->isChecked()) m_pCheckBox_AxsunOctControl->setChecked(false);
    if (m_pCheckBox_ZaberStageControl->isChecked()) m_pCheckBox_ZaberStageControl->setChecked(false);
	if (m_pCheckBox_FaulhaberMotorControl->isChecked()) m_pCheckBox_FaulhaberMotorControl->setChecked(false);
}


void QDeviceControlTab::createFlimSynchronizationControl()
{
    // Create widgets for FLIM laser control
    QGridLayout *pGridLayout_FlimSynchronizationControl = new QGridLayout;
    pGridLayout_FlimSynchronizationControl->setSpacing(3);

    m_pCheckBox_FlimControl = new QCheckBox(m_pGroupBox_FlimControl);
#ifndef NI_ENABLE
    m_pCheckBox_FlimControl->setText("Connect to Arduino MCU for FLIm Control");
#else
	m_pCheckBox_FlimControl->setText("Connect to NIDAQmx for FLIm Control");
#endif

    m_pToggleButton_FlimSynchronization = new QPushButton(m_pGroupBox_FlimControl);
    m_pToggleButton_FlimSynchronization->setText("FLIm Laser Sync On");
    m_pToggleButton_FlimSynchronization->setFixedWidth(120);
    m_pToggleButton_FlimSynchronization->setCheckable(true);
    m_pToggleButton_FlimSynchronization->setDisabled(true);
	
    m_pSlider_FlimSyncAdjust = new QSlider(m_pGroupBox_FlimControl);
	m_pSlider_FlimSyncAdjust->setOrientation(Qt::Horizontal);
	m_pSlider_FlimSyncAdjust->setRange(0, FLIM_ALINES - 1);
    m_pSlider_FlimSyncAdjust->setValue(m_pConfig->flimSyncAdjust);
	m_pSlider_FlimSyncAdjust->setDisabled(true);

    m_pLabel_FlimSyncAdjust = new QLabel(QString(" Sync Adjust (%1)").arg(m_pConfig->flimSyncAdjust, 3, 10), m_pGroupBox_FlimControl);
	m_pLabel_FlimSyncAdjust->setFixedWidth(90);
	m_pLabel_FlimSyncAdjust->setBuddy(m_pSlider_FlimSyncAdjust);
	m_pLabel_FlimSyncAdjust->setDisabled(true);

    // Connect signal and slot
    connect(m_pCheckBox_FlimControl, SIGNAL(toggled(bool)), this, SLOT(connectFlimControl(bool)));
    connect(m_pToggleButton_FlimSynchronization, SIGNAL(toggled(bool)), this, SLOT(startFlimSynchronization(bool)));
	connect(m_pSlider_FlimSyncAdjust, SIGNAL(valueChanged(int)), this, SLOT(setFlimSyncAdjust(int)));
}

void QDeviceControlTab::createPmtGainControl()
{
    // Create widgets for PMT gain control
    QGridLayout *pGridLayout_FlimSyncPmtGainControl = new QGridLayout;
	pGridLayout_FlimSyncPmtGainControl->setSpacing(3);

#ifndef NI_ENABLE
    m_pSpinBox_PmtGainControl = new QDoubleSpinBox(m_pGroupBox_FlimControl);
    m_pSpinBox_PmtGainControl->setRange(0, ARDUINO_DAC_STEP_SIZE * 4095.0 + ARDUINO_DAC_OFFSET);
    m_pSpinBox_PmtGainControl->setSingleStep(ARDUINO_DAC_STEP_SIZE);
    m_pSpinBox_PmtGainControl->setDecimals(3);
    m_pSpinBox_PmtGainControl->setValue((double)m_pConfig->pmtGainVoltageLevel * ARDUINO_DAC_STEP_SIZE + ARDUINO_DAC_OFFSET);
    m_pSpinBox_PmtGainControl->setAlignment(Qt::AlignCenter);
    m_pSpinBox_PmtGainControl->setFixedWidth(50);
    m_pSpinBox_PmtGainControl->setDisabled(true);
#else
	m_pLineEdit_PmtGainVoltage = new QLineEdit(m_pGroupBox_FlimControl);
	m_pLineEdit_PmtGainVoltage->setFixedWidth(50);
	m_pLineEdit_PmtGainVoltage->setText(QString::number(m_pConfig->pmtGainVoltage, 'f', 3));
	m_pLineEdit_PmtGainVoltage->setAlignment(Qt::AlignCenter);
	m_pLineEdit_PmtGainVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_PmtGainVoltage->setDisabled(true);
#endif

    m_pLabel_PmtGainControl = new QLabel("PMT Gain  ", m_pGroupBox_FlimControl);
#ifndef NI_ENABLE
    m_pLabel_PmtGainControl->setBuddy(m_pSpinBox_PmtGainControl);
#else
	m_pLabel_PmtGainControl->setBuddy(m_pLineEdit_PmtGainVoltage);
#endif
    m_pLabel_PmtGainControl->setDisabled(true);

    m_pLabel_PmtGainVoltage = new QLabel("V", m_pGroupBox_FlimControl);
#ifndef NI_ENABLE
	m_pLabel_PmtGainVoltage->setBuddy(m_pSpinBox_PmtGainControl);
#else
	m_pLabel_PmtGainVoltage->setBuddy(m_pLineEdit_PmtGainVoltage);
#endif
    m_pLabel_PmtGainVoltage->setDisabled(true);

    m_pToggleButton_PmtGainVoltage = new QPushButton(m_pGroupBox_FlimControl);
    m_pToggleButton_PmtGainVoltage->setText("On");
    m_pToggleButton_PmtGainVoltage->setFixedWidth(30);
    m_pToggleButton_PmtGainVoltage->setCheckable(true);
    m_pToggleButton_PmtGainVoltage->setDisabled(true);

	pGridLayout_FlimSyncPmtGainControl->addWidget(m_pCheckBox_FlimControl, 0, 0, 1, 7);
	pGridLayout_FlimSyncPmtGainControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_FlimSyncPmtGainControl->addWidget(m_pToggleButton_FlimSynchronization, 1, 1);
	pGridLayout_FlimSyncPmtGainControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 2);
	pGridLayout_FlimSyncPmtGainControl->addWidget(m_pLabel_PmtGainControl, 1, 3);
#ifndef NI_ENABLE
	pGridLayout_FlimSyncPmtGainControl->addWidget(m_pSpinBox_PmtGainControl, 1, 4);
#else
	pGridLayout_FlimSyncPmtGainControl->addWidget(m_pLineEdit_PmtGainVoltage, 1, 4);
#endif
	pGridLayout_FlimSyncPmtGainControl->addWidget(m_pLabel_PmtGainVoltage, 1, 5);
	pGridLayout_FlimSyncPmtGainControl->addWidget(m_pToggleButton_PmtGainVoltage, 1, 6);
	
	QHBoxLayout* pHBoxLayout_FlimSyncAdjust = new QHBoxLayout;
	pHBoxLayout_FlimSyncAdjust->addWidget(m_pLabel_FlimSyncAdjust);
	pHBoxLayout_FlimSyncAdjust->addWidget(m_pSlider_FlimSyncAdjust);

	pGridLayout_FlimSyncPmtGainControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 0);
	pGridLayout_FlimSyncPmtGainControl->addItem(pHBoxLayout_FlimSyncAdjust, 2, 1, 1, 6);

    m_pVBoxLayout_FlimControl->addItem(pGridLayout_FlimSyncPmtGainControl);

    // Connect signal and slot
#ifndef NI_ENABLE
    connect(m_pSpinBox_PmtGainControl, SIGNAL(valueChanged(double)), this, SLOT(changePmtGainVoltage(double)));
#else
	connect(m_pLineEdit_PmtGainVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changePmtGainVoltage(const QString &)));
#endif
    connect(m_pToggleButton_PmtGainVoltage, SIGNAL(toggled(bool)), this, SLOT(applyPmtGainVoltage(bool)));
}

void QDeviceControlTab::createFlimLaserPowerControl()
{
    // Create widgets for FLIM laser power control
    QGridLayout *pGridLayout_FlimLaserPowerControl = new QGridLayout;
    pGridLayout_FlimLaserPowerControl->setSpacing(3);

    m_pCheckBox_FlimLaserPowerControl = new QCheckBox(m_pGroupBox_FlimControl);
    m_pCheckBox_FlimLaserPowerControl->setText("Connect to FLIm Laser for Power Control");

	m_pSpinBox_FlimLaserPowerControl = new QSpinBox(m_pGroupBox_FlimControl);
	m_pSpinBox_FlimLaserPowerControl->setValue(0);
	m_pSpinBox_FlimLaserPowerControl->setRange(-10000, 10000);
	m_pSpinBox_FlimLaserPowerControl->setFixedWidth(15);
	m_pSpinBox_FlimLaserPowerControl->setDisabled(true);
	static int flim_laser_power_level = 0;

    pGridLayout_FlimLaserPowerControl->addWidget(m_pCheckBox_FlimLaserPowerControl, 0, 0);
    pGridLayout_FlimLaserPowerControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 1);
    pGridLayout_FlimLaserPowerControl->addWidget(m_pSpinBox_FlimLaserPowerControl, 0, 2);
	
	m_pVBoxLayout_FlimControl->addItem(pGridLayout_FlimLaserPowerControl);

	// Connect signal and slot
    connect(m_pCheckBox_FlimLaserPowerControl, SIGNAL(toggled(bool)), this, SLOT(connectFlimLaser(bool)));
	connect(m_pSpinBox_FlimLaserPowerControl, SIGNAL(valueChanged(int)), this, SLOT(adjustLaserPower(int)));
}

void QDeviceControlTab::createFLimCalibControl()
{
    // Create widgets for FLIm Calibration
    QGridLayout *pGridLayout_FlimCalibration = new QGridLayout;
    pGridLayout_FlimCalibration->setSpacing(3);

    m_pCheckBox_PX14DigitizerControl = new QCheckBox(m_pGroupBox_FlimControl);
    m_pCheckBox_PX14DigitizerControl->setText("Connect to PX14 Digitizer");
    m_pCheckBox_PX14DigitizerControl->setDisabled(true);

    m_pSlider_PX14DcOffset = new QSlider(m_pGroupBox_FlimControl);
    m_pSlider_PX14DcOffset->setOrientation(Qt::Horizontal);
    m_pSlider_PX14DcOffset->setRange(0, 4095);
    m_pSlider_PX14DcOffset->setValue(m_pConfig->px14DcOffset);
    m_pSlider_PX14DcOffset->setDisabled(true);

    m_pLabel_PX14DcOffset = new QLabel(QString("Set DC Offset (%1)").arg(m_pConfig->px14DcOffset, 4, 10), m_pGroupBox_FlimControl);
    m_pLabel_PX14DcOffset->setBuddy(m_pSlider_PX14DcOffset);
    m_pLabel_PX14DcOffset->setDisabled(true);

    m_pPushButton_FlimCalibration = new QPushButton(this);
    m_pPushButton_FlimCalibration->setText("FLIm Pulse View and Calibration...");
    m_pPushButton_FlimCalibration->setFixedWidth(200);
    m_pPushButton_FlimCalibration->setDisabled(true);

    pGridLayout_FlimCalibration->addWidget(m_pCheckBox_PX14DigitizerControl, 0, 0, 1, 3);
    pGridLayout_FlimCalibration->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 0);
    pGridLayout_FlimCalibration->addWidget(m_pLabel_PX14DcOffset, 1, 1);
    pGridLayout_FlimCalibration->addWidget(m_pSlider_PX14DcOffset, 1, 2);
    pGridLayout_FlimCalibration->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 0);
    pGridLayout_FlimCalibration->addWidget(m_pPushButton_FlimCalibration, 2, 1 , 1, 2);

    m_pVBoxLayout_FlimControl->addItem(pGridLayout_FlimCalibration);

    m_pGroupBox_FlimControl->setLayout(m_pVBoxLayout_FlimControl);
    m_pGroupBox_FlimControl->resize(m_pVBoxLayout_FlimControl->minimumSize());
    m_pVBoxLayout->addWidget(m_pGroupBox_FlimControl);

    // Connect signal and slot
    connect(m_pCheckBox_PX14DigitizerControl, SIGNAL(toggled(bool)), this, SLOT(connectPX14Digitizer(bool)));
    connect(m_pSlider_PX14DcOffset, SIGNAL(valueChanged(int)), this, SLOT(setPX14DcOffset(int)));
    connect(m_pPushButton_FlimCalibration, SIGNAL(clicked(bool)), this, SLOT(createFlimCalibDlg()));
}

void QDeviceControlTab::createAxsunOctControl()
{
    // Create widgets for Axsun OCT control
    QGroupBox *pGroupBox_AxsunOctControl = new QGroupBox;
    QVBoxLayout *pVBoxLayout_AxsunOctControl = new QVBoxLayout;
    pVBoxLayout_AxsunOctControl->setSpacing(3);

	m_pCheckBox_AxsunOctControl = new QCheckBox(pGroupBox_AxsunOctControl);
	m_pCheckBox_AxsunOctControl->setText("Connect to Axsun OCT Control");

    m_pToggleButton_LightSource = new QPushButton(this);
    m_pToggleButton_LightSource->setCheckable(true);
    m_pToggleButton_LightSource->setFixedWidth(60);
    m_pToggleButton_LightSource->setText("Laser On");
    m_pToggleButton_LightSource->setDisabled(true);
	m_pToggleButton_LiveImaging = new QPushButton(this);
	m_pToggleButton_LiveImaging->setCheckable(true);
	m_pToggleButton_LiveImaging->setFixedWidth(70);
	m_pToggleButton_LiveImaging->setText("Imaging On");
	m_pToggleButton_LiveImaging->setDisabled(true);

///    m_pPushButton_SetBackground = new QPushButton(this);
///    m_pPushButton_SetBackground->setFixedSize(30, 20);
///    m_pPushButton_SetBackground->setText("Set");
///    m_pPushButton_SetBackground->setDisabled(true);
///    m_pPushButton_ResetBackground = new QPushButton(this);
///    m_pPushButton_ResetBackground->setFixedSize(40, 20);
///    m_pPushButton_ResetBackground->setText("Reset");
///    m_pPushButton_ResetBackground->setDisabled(true);
///    m_pLabel_Background = new QLabel(" Background ", this);
///    m_pLabel_Background->setBuddy(m_pPushButton_SetBackground);
///    m_pLabel_Background->setDisabled(true);
///    m_pLineEdit_a2 = new QLineEdit(this);
///    m_pLineEdit_a2->setFixedWidth(25);
///    m_pLineEdit_a2->setText(QString::number(m_pConfig->axsunDisComA2));
///    m_pLineEdit_a2->setAlignment(Qt::AlignCenter);
///    m_pLineEdit_a2->setDisabled(true);
///    m_pLineEdit_a3 = new QLineEdit(this);
///    m_pLineEdit_a3->setFixedWidth(25);
///    m_pLineEdit_a3->setText(QString::number(m_pConfig->axsunDisComA3));
///    m_pLineEdit_a3->setAlignment(Qt::AlignCenter);
///    m_pLineEdit_a3->setDisabled(true);
///    m_pPushButton_DispComp = new QPushButton(this);
///    m_pPushButton_DispComp->setMinimumSize(95, 20);
///    m_pPushButton_DispComp->setText("DisCom Apply");
///    m_pPushButton_DispComp->setDisabled(true);
///    m_pLabel_DispCompCoef = new QLabel("DisCom Coeff. (a2, a3)  ", this);
///    m_pLabel_DispCompCoef->setBuddy(m_pPushButton_DispComp);
///    m_pLabel_DispCompCoef->setDisabled(true);

    m_pSpinBox_ClockDelay = new QMySpinBox(this);
    m_pSpinBox_ClockDelay->setFixedWidth(60);
    m_pSpinBox_ClockDelay->setRange(28.285, 64.580);
    m_pSpinBox_ClockDelay->setSingleStep(0.576112);
    m_pSpinBox_ClockDelay->setValue(CLOCK_GAIN * (double)m_pConfig->axsunClockDelay + CLOCK_OFFSET);
    m_pSpinBox_ClockDelay->setDecimals(3);
    m_pSpinBox_ClockDelay->setAlignment(Qt::AlignCenter);
    m_pSpinBox_ClockDelay->setDisabled(true);
    m_pLabel_ClockDelay = new QLabel("   Clock Delay  ", this);
    m_pLabel_ClockDelay->setBuddy(m_pSpinBox_ClockDelay);
    m_pLabel_ClockDelay->setDisabled(true);

    m_pSpinBox_VDLLength = new QMySpinBox(this);
    m_pSpinBox_VDLLength->setFixedWidth(55);
    m_pSpinBox_VDLLength->setRange(0.00, 15.00);
    m_pSpinBox_VDLLength->setSingleStep(0.01);
    m_pSpinBox_VDLLength->setValue(m_pConfig->axsunVDLLength);
    m_pSpinBox_VDLLength->setDecimals(2);
    m_pSpinBox_VDLLength->setAlignment(Qt::AlignCenter);
    m_pSpinBox_VDLLength->setDisabled(true);
    m_pLabel_VDLLength = new QLabel("   VDL Length  ", this);
    m_pLabel_VDLLength->setBuddy(m_pSpinBox_VDLLength);
    m_pLabel_VDLLength->setDisabled(true);
	m_pPushButton_VDLHome = new QPushButton(this);
	m_pPushButton_VDLHome->setFixedWidth(40);
	m_pPushButton_VDLHome->setText("Home");
	m_pPushButton_VDLHome->setDisabled(true);

    m_pLineEdit_DecibelMax = new QLineEdit(this);
    m_pLineEdit_DecibelMax->setFixedWidth(30);
    m_pLineEdit_DecibelMax->setText(QString::number(m_pConfig->axsunDbRange.max));
    m_pLineEdit_DecibelMax->setAlignment(Qt::AlignCenter);
    m_pLineEdit_DecibelMax->setDisabled(true);
    m_pLineEdit_DecibelMin = new QLineEdit(this);
    m_pLineEdit_DecibelMin->setFixedWidth(30);
    m_pLineEdit_DecibelMin->setText(QString::number(m_pConfig->axsunDbRange.min));
    m_pLineEdit_DecibelMin->setAlignment(Qt::AlignCenter);
    m_pLineEdit_DecibelMin->setDisabled(true);

    uint8_t color[256];
       for (int i = 0; i < 256; i++)
           color[i] = (uint8_t)i;

    m_pImageView_Colorbar = new QImageView(ColorTable::colortable(m_pConfig->octColorTable), 256, 1);
    m_pImageView_Colorbar->setFixedSize(150, 15);
    m_pImageView_Colorbar->drawImage(color);
    m_pImageView_Colorbar->setDisabled(true);
    m_pLabel_DecibelRange = new QLabel("dB Range ", this);
    m_pLabel_DecibelRange->setFixedWidth(65);
    m_pLabel_DecibelRange->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    m_pLabel_DecibelRange->setBuddy(m_pImageView_Colorbar);
    m_pLabel_DecibelRange->setDisabled(true);

    // Set layout
    QHBoxLayout *pHBoxLayout_LightSource = new QHBoxLayout;
    pHBoxLayout_LightSource->setSpacing(1);

    pHBoxLayout_LightSource->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
    pHBoxLayout_LightSource->addWidget(m_pToggleButton_LightSource);
	pHBoxLayout_LightSource->addWidget(m_pToggleButton_LiveImaging);
///    pHBoxLayout_LightSource->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
///    pHBoxLayout_LightSource->addWidget(m_pLabel_Background);
///    pHBoxLayout_LightSource->addWidget(m_pPushButton_SetBackground);
///    pHBoxLayout_LightSource->addWidget(m_pPushButton_ResetBackground);

    QHBoxLayout *pHBoxLayout_Calibration = new QHBoxLayout;
    pHBoxLayout_Calibration->setSpacing(1);

///    pHBoxLayout_Calibration->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
///    pHBoxLayout_Calibration->addWidget(m_pLabel_DispCompCoef);
///    pHBoxLayout_Calibration->addWidget(m_pLineEdit_a2);
///    pHBoxLayout_Calibration->addWidget(m_pLineEdit_a3);
///    pHBoxLayout_Calibration->addWidget(m_pPushButton_DispComp);

    QHBoxLayout *pHBoxLayout_DeviceControl = new QHBoxLayout;
    pHBoxLayout_DeviceControl->setSpacing(1);

    pHBoxLayout_DeviceControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
    pHBoxLayout_DeviceControl->addWidget(m_pLabel_ClockDelay);
    pHBoxLayout_DeviceControl->addWidget(m_pSpinBox_ClockDelay);
    pHBoxLayout_DeviceControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
    pHBoxLayout_DeviceControl->addWidget(m_pLabel_VDLLength);
    pHBoxLayout_DeviceControl->addWidget(m_pSpinBox_VDLLength);
	pHBoxLayout_DeviceControl->addWidget(m_pPushButton_VDLHome);

    QHBoxLayout *pHBoxLayout_Decibel = new QHBoxLayout;
    pHBoxLayout_Decibel->setSpacing(1);

    pHBoxLayout_Decibel->addWidget(m_pLabel_DecibelRange);
    pHBoxLayout_Decibel->addWidget(m_pLineEdit_DecibelMin);
    pHBoxLayout_Decibel->addWidget(m_pImageView_Colorbar);
    pHBoxLayout_Decibel->addWidget(m_pLineEdit_DecibelMax);

    pVBoxLayout_AxsunOctControl->addWidget(m_pCheckBox_AxsunOctControl);
    pVBoxLayout_AxsunOctControl->addItem(pHBoxLayout_LightSource);
    pVBoxLayout_AxsunOctControl->addItem(pHBoxLayout_Calibration);
    pVBoxLayout_AxsunOctControl->addItem(pHBoxLayout_DeviceControl);
    pVBoxLayout_AxsunOctControl->addItem(pHBoxLayout_Decibel);

    pGroupBox_AxsunOctControl->setLayout(pVBoxLayout_AxsunOctControl);
    m_pVBoxLayout->addWidget(pGroupBox_AxsunOctControl);

    // Connect signal and slot
	connect(m_pCheckBox_AxsunOctControl, SIGNAL(toggled(bool)), this, SLOT(connectAxsunControl(bool)));
	connect(m_pToggleButton_LightSource, SIGNAL(toggled(bool)), this, SLOT(setLightSource(bool)));
	connect(m_pToggleButton_LiveImaging, SIGNAL(toggled(bool)), this, SLOT(setLiveImaging(bool)));
    ///connect(m_pPushButton_SetBackground, SIGNAL(clicked(bool)), this, SLOT(setBackground()));
    ///connect(m_pPushButton_ResetBackground, SIGNAL(clicked(bool)), this, SLOT(resetBackground()));
    ///connect(m_pPushButton_DispComp, SIGNAL(clicked(bool)), this, SLOT(setDispComp()));
	connect(m_pSpinBox_ClockDelay, SIGNAL(valueChanged(double)), this, SLOT(setClockDelay(double)));
	connect(m_pSpinBox_VDLLength, SIGNAL(valueChanged(double)), this, SLOT(setVDLLength(double)));
	connect(m_pPushButton_VDLHome, SIGNAL(clicked(bool)), this, SLOT(setVDLHome()));
	connect(m_pLineEdit_DecibelMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDecibelRange()));
	connect(m_pLineEdit_DecibelMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDecibelRange()));
	connect(this, SIGNAL(transferAxsunArray(int)), m_pStreamTab->getOperationTab()->getProgressBar(), SLOT(setValue(int)));
}

void QDeviceControlTab::createZaberStageControl()
{
    // Create widgets for Zaber stage control
    QGroupBox *pGroupBox_ZaberStageControl = new QGroupBox;
    QGridLayout *pGridLayout_ZaberStageControl = new QGridLayout;
    pGridLayout_ZaberStageControl->setSpacing(3);

    m_pCheckBox_ZaberStageControl = new QCheckBox(pGroupBox_ZaberStageControl);
    m_pCheckBox_ZaberStageControl->setText("Connect to Zaber Stage");

    m_pPushButton_SetTargetSpeed = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_SetTargetSpeed->setText("Target Speed");
    m_pPushButton_SetTargetSpeed->setFixedWidth(100);
    m_pPushButton_SetTargetSpeed->setDisabled(true);
    m_pPushButton_MoveAbsolute = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_MoveAbsolute->setText("Move Absolute");
    m_pPushButton_MoveAbsolute->setFixedWidth(100);
    m_pPushButton_MoveAbsolute->setDisabled(true);
    m_pPushButton_Home = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_Home->setText("Home");
    m_pPushButton_Home->setFixedWidth(60);
    m_pPushButton_Home->setDisabled(true);
    m_pPushButton_Stop = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_Stop->setText("Stop");
    m_pPushButton_Stop->setFixedWidth(60);
    m_pPushButton_Stop->setDisabled(true);

    m_pLineEdit_TargetSpeed = new QLineEdit(pGroupBox_ZaberStageControl);
    m_pLineEdit_TargetSpeed->setFixedWidth(25);
    m_pLineEdit_TargetSpeed->setText(QString::number(m_pConfig->zaberPullbackSpeed));
    m_pLineEdit_TargetSpeed->setAlignment(Qt::AlignCenter);
    m_pLineEdit_TargetSpeed->setDisabled(true);
    m_pLineEdit_TravelLength = new QLineEdit(pGroupBox_ZaberStageControl);
    m_pLineEdit_TravelLength->setFixedWidth(25);
    m_pLineEdit_TravelLength->setText(QString::number(m_pConfig->zaberPullbackLength));
    m_pLineEdit_TravelLength->setAlignment(Qt::AlignCenter);
    m_pLineEdit_TravelLength->setDisabled(true);

    m_pLabel_TargetSpeed = new QLabel("mm/s", pGroupBox_ZaberStageControl);
    m_pLabel_TargetSpeed->setBuddy(m_pLineEdit_TargetSpeed);
    m_pLabel_TargetSpeed->setDisabled(true);
    m_pLabel_TravelLength = new QLabel("mm", pGroupBox_ZaberStageControl);
    m_pLabel_TravelLength->setBuddy(m_pLineEdit_TravelLength);
    m_pLabel_TravelLength->setDisabled(true);

    pGridLayout_ZaberStageControl->addWidget(m_pCheckBox_ZaberStageControl, 0, 0, 1, 5);

    pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 0);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_Home, 1, 1);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_SetTargetSpeed, 1, 2);
    pGridLayout_ZaberStageControl->addWidget(m_pLineEdit_TargetSpeed, 1, 3);
    pGridLayout_ZaberStageControl->addWidget(m_pLabel_TargetSpeed, 1, 4);

    pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 0);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_Stop, 2, 1);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_MoveAbsolute, 2, 2);
    pGridLayout_ZaberStageControl->addWidget(m_pLineEdit_TravelLength, 2, 3);
    pGridLayout_ZaberStageControl->addWidget(m_pLabel_TravelLength, 2, 4);

    pGroupBox_ZaberStageControl->setLayout(pGridLayout_ZaberStageControl);
    m_pVBoxLayout->addWidget(pGroupBox_ZaberStageControl);

    // Connect signal and slot
    connect(m_pCheckBox_ZaberStageControl, SIGNAL(toggled(bool)), this, SLOT(connectZaberStage(bool)));
    connect(m_pPushButton_MoveAbsolute, SIGNAL(clicked(bool)), this, SLOT(moveAbsolute()));
    connect(m_pLineEdit_TargetSpeed, SIGNAL(textChanged(const QString &)), this, SLOT(setTargetSpeed(const QString &)));
    connect(m_pLineEdit_TravelLength, SIGNAL(textChanged(const QString &)), this, SLOT(changeZaberPullbackLength(const QString &)));
    connect(m_pPushButton_Home, SIGNAL(clicked(bool)), this, SLOT(home()));
    connect(m_pPushButton_Stop, SIGNAL(clicked(bool)), this, SLOT(stop()));
}

void QDeviceControlTab::createFaulhaberMotorControl()
{
    // Create widgets for Faulhaber motor control
    QGroupBox *pGroupBox_FaulhaberMotorControl = new QGroupBox;
    QGridLayout *pGridLayout_FaulhaberMotorControl = new QGridLayout;
    pGridLayout_FaulhaberMotorControl->setSpacing(3);

    m_pCheckBox_FaulhaberMotorControl = new QCheckBox(pGroupBox_FaulhaberMotorControl);
    m_pCheckBox_FaulhaberMotorControl->setText("Connect to Faulhaber Motor");

    m_pToggleButton_Rotate = new QPushButton(pGroupBox_FaulhaberMotorControl);
    m_pToggleButton_Rotate->setText("Rotate");
    m_pToggleButton_Rotate->setCheckable(pGroupBox_FaulhaberMotorControl);
	m_pToggleButton_Rotate->setDisabled(true);

    m_pLineEdit_RPM = new QLineEdit(pGroupBox_FaulhaberMotorControl);
    m_pLineEdit_RPM->setFixedWidth(40);
    m_pLineEdit_RPM->setText(QString::number(m_pConfig->faulhaberRpm));
	m_pLineEdit_RPM->setAlignment(Qt::AlignCenter);
    m_pLineEdit_RPM->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_RPM->setDisabled(true);

    m_pLabel_RPM = new QLabel("RPM", pGroupBox_FaulhaberMotorControl);
    m_pLabel_RPM->setBuddy(m_pLineEdit_RPM);
	m_pLabel_RPM->setDisabled(true);

    pGridLayout_FaulhaberMotorControl->addWidget(m_pCheckBox_FaulhaberMotorControl, 0, 0, 1, 4);
    pGridLayout_FaulhaberMotorControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 0);
    pGridLayout_FaulhaberMotorControl->addWidget(m_pToggleButton_Rotate, 1, 1);
    pGridLayout_FaulhaberMotorControl->addWidget(m_pLineEdit_RPM, 1, 2);
    pGridLayout_FaulhaberMotorControl->addWidget(m_pLabel_RPM, 1, 3);

    pGroupBox_FaulhaberMotorControl->setLayout(pGridLayout_FaulhaberMotorControl);
    m_pVBoxLayout->addWidget(pGroupBox_FaulhaberMotorControl);
	m_pVBoxLayout->addStretch(1);

	// Connect signal and slot
    connect(m_pCheckBox_FaulhaberMotorControl, SIGNAL(toggled(bool)), this, SLOT(connectFaulhaberMotor(bool)));
	connect(m_pToggleButton_Rotate, SIGNAL(toggled(bool)), this, SLOT(rotate(bool)));
	connect(m_pLineEdit_RPM, SIGNAL(textChanged(const QString &)), this, SLOT(changeFaulhaberRpm(const QString &)));
}



void QDeviceControlTab::connectFlimControl(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_FlimControl->setText("Disconnect from NIDAQmx");

#ifndef NI_ENABLE
        // Create Arduino MCU control objects for FLIm control
		if (!m_pArduinoMCU)
		{
			m_pArduinoMCU = new ArduinoMCU;
			m_pArduinoMCU->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				qmsg.replace('\n', ' ');
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}

        // Connect the Arduino MCU
        if (!(m_pArduinoMCU->ConnectDevice()))
        {
            m_pCheckBox_FlimControl->setChecked(false);
            return;
        }
#endif

        // Set enabled true for FLIm control widgets
        m_pToggleButton_FlimSynchronization->setEnabled(true);
        m_pLabel_PmtGainControl->setEnabled(true);
#ifndef NI_ENABLE
        m_pSpinBox_PmtGainControl->setEnabled(true);
#else
		m_pLineEdit_PmtGainVoltage->setEnabled(true);
#endif
        m_pLabel_PmtGainVoltage->setEnabled(true);
        m_pToggleButton_PmtGainVoltage->setEnabled(true);
		m_pLabel_FlimSyncAdjust->setEnabled(true);
		m_pSlider_FlimSyncAdjust->setEnabled(true);

		//// Set buttons
		//m_pToggleButton_FlimSynchronization->setChecked(true);
		//m_pToggleButton_PmtGainVoltage->setChecked(true);
    }
    else
    {
        // Set enabled false for FLIm control widgets
        m_pToggleButton_FlimSynchronization->setEnabled(false);
        m_pLabel_PmtGainControl->setEnabled(false);
#ifndef NI_ENABLE
		m_pSpinBox_PmtGainControl->setEnabled(false);
#else
		m_pLineEdit_PmtGainVoltage->setEnabled(false);
#endif
        m_pLabel_PmtGainVoltage->setEnabled(false);
        m_pToggleButton_PmtGainVoltage->setEnabled(false);
		m_pLabel_FlimSyncAdjust->setEnabled(false);
		m_pSlider_FlimSyncAdjust->setEnabled(false);

#ifndef NI_ENABLE
        if (m_pArduinoMCU)
        {
            // Disconnect the MCU
            m_pArduinoMCU->DisconnectDevice();

            // Delete Arduino MCU control objects
            delete m_pArduinoMCU;
            m_pArduinoMCU = nullptr;
        }
#else

#endif

        // Set buttons
        m_pToggleButton_FlimSynchronization->setChecked(false);
		m_pToggleButton_FlimSynchronization->setText("FLIm Laser Sync On");
        m_pToggleButton_PmtGainVoltage->setChecked(false);
		m_pToggleButton_PmtGainVoltage->setText("On");

        // Set text
        m_pCheckBox_FlimControl->setText("Connect to NIDAQmx for FLIm Control");
    }
}

void QDeviceControlTab::startFlimSynchronization(bool toggled)
{
#ifndef NI_ENABLE
    if (m_pArduinoMCU)
    {
        if (toggled)
        {
            // Set text
            m_pToggleButton_FlimSynchronization->setText("FLIm Laser Sync Off");

            // Start FLIm-OCT synchronization
            m_pArduinoMCU->SyncDevice(true);
        }
        else
        {
            // Stop FLIm-OCT synchronization
            m_pArduinoMCU->SyncDevice(false);

            // Set text
            m_pToggleButton_FlimSynchronization->setText("FLIm Laser Sync On");
        }
    }
#else
	if (toggled)
	{
		// Set text
		m_pToggleButton_FlimSynchronization->setText("FLIm Laser Sync Off");

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
		if ((!m_pFlimFreqDivider->initialize()) || (!m_pAxsunFreqDivider->initialize()))
		{
			m_pToggleButton_FlimSynchronization->setChecked(false);
			return;
		}

		// Generate FLIm laser & Axsun OCT sync
		m_pFlimFreqDivider->start();
		m_pAxsunFreqDivider->start();
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
		m_pToggleButton_FlimSynchronization->setText("FLIm Laser Sync On");
	}
#endif
}

void QDeviceControlTab::setFlimSyncAdjust(int adjust)
{
	m_pConfig->flimSyncAdjust = adjust;
	m_pLabel_FlimSyncAdjust->setText(QString(" Sync Adjust (%1)").arg(m_pConfig->flimSyncAdjust, 3, 10));
}

void QDeviceControlTab::applyPmtGainVoltage(bool toggled)
{
	// Set text
	m_pToggleButton_PmtGainVoltage->setText(toggled ? "Off" : "On");

#ifndef NI_ENABLE
    if (m_pArduinoMCU)
        // Generate PMT gain voltage
        changePmtGainVoltage(m_pSpinBox_PmtGainControl->value());
#else
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
#endif
 }

#ifndef NI_ENABLE
void QDeviceControlTab::changePmtGainVoltage(double gain)
{
    if (m_pArduinoMCU)
    {
        int gain_level = int((gain - ARDUINO_DAC_OFFSET) / (ARDUINO_DAC_STEP_SIZE));

        if (m_pToggleButton_PmtGainVoltage->isChecked())
        {
            m_pArduinoMCU->AdjustGainLevel(gain_level);
            m_pConfig->pmtGainVoltageLevel = gain_level;
        }
        else
            m_pArduinoMCU->AdjustGainLevel(0);
    }
}
#else
void QDeviceControlTab::changePmtGainVoltage(const QString & str)
{
	m_pConfig->pmtGainVoltage = str.toFloat();
}
#endif

void QDeviceControlTab::connectFlimLaser(bool toggled)
{
	if (toggled)
	{
		// Set text
        m_pCheckBox_FlimLaserPowerControl->setText("Disconnect from FLIm Laser");

		// Create FLIM laser power control objects
		if (!m_pElforlightLaser)
		{
			m_pElforlightLaser = new ElforlightLaser;
			m_pElforlightLaser->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				qmsg.replace('\n', ' ');
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}

		// Connect the laser
		if (!(m_pElforlightLaser->ConnectDevice()))
		{
			m_pCheckBox_FlimLaserPowerControl->setChecked(false);
			return;
		}

		// Set enabled true for FLIM laser power control widgets
		m_pSpinBox_FlimLaserPowerControl->setEnabled(true);
	}
	else
	{
		// Set enabled false for FLIM laser power control widgets
		m_pSpinBox_FlimLaserPowerControl->setEnabled(false);

		if (m_pElforlightLaser)
		{
			// Disconnect the laser
			m_pElforlightLaser->DisconnectDevice();

			// Delete FLIM laser power control objects
			delete m_pElforlightLaser;
            m_pElforlightLaser = nullptr;
		}
		
		// Set text
        m_pCheckBox_FlimLaserPowerControl->setText("Connect to FLIm Laser for Power Control");
	}
}

void QDeviceControlTab::adjustLaserPower(int level)
{
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


void QDeviceControlTab::connectPX14Digitizer(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_PX14DigitizerControl->setText("Disconnect from PX14 Digitizer");

        // Connect the digitizer
//        DataAcquisition *pDataAcq = m_pStreamTab->getOperationTab()->m_pDataAcquisition;
//        if (!pDataAcq->InitializeAcquistion())
//            return;

        // Set enabled true for PX14 digitizer widgets
        m_pLabel_PX14DcOffset->setEnabled(true);
        m_pSlider_PX14DcOffset->setEnabled(true);
        m_pPushButton_FlimCalibration->setEnabled(true);
    }
    else
    {
        // Set disabled true for PX14 digitizer widgets
        m_pLabel_PX14DcOffset->setDisabled(true);
        m_pSlider_PX14DcOffset->setDisabled(true);
        m_pPushButton_FlimCalibration->setDisabled(true);

        // Close FLIm calibration window
        if (m_pFlimCalibDlg) m_pFlimCalibDlg->close();

        // Set text
        m_pCheckBox_PX14DigitizerControl->setText("Connect to PX14 Digitizer");
    }

}

void QDeviceControlTab::setPX14DcOffset(int offset)
{    
    DataAcquisition *pDataAcq = m_pStreamTab->getOperationTab()->m_pDataAcquisition;
    pDataAcq->SetDcOffset(offset);

    m_pConfig->px14DcOffset = offset;
    m_pLabel_PX14DcOffset->setText(QString("Set DC Offset (%1)").arg(m_pConfig->px14DcOffset, 4, 10));
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
//        emit m_pFlimCalibDlg->plotRoiPulse(m_pFLIm, m_pSlider_SelectAline->value() / 4);
    }
    m_pFlimCalibDlg->raise();
    m_pFlimCalibDlg->activateWindow();
}

void QDeviceControlTab::deleteFlimCalibDlg()
{
//    m_pFlimCalibDlg->showWindow(false);
//    m_pFlimCalibDlg->showMeanDelay(false);
//    m_pFlimCalibDlg->showMask(false);

    m_pFlimCalibDlg->deleteLater();
    m_pFlimCalibDlg = nullptr;
}


void QDeviceControlTab::connectAxsunControl(bool toggled)
{
	if (toggled)
	{
		// Set text
		m_pCheckBox_AxsunOctControl->setText("Disconnect from Axsun OCT Control");

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
		}

		// Initialize the Axsun OCT control
		if (!(m_pAxsunControl->initialize()))
		{
			m_pCheckBox_AxsunOctControl->setChecked(false);
			return;
		}

        ///std::thread setup([&]()
        ///{
			// Default Bypass Mode
			m_pAxsunControl->setBypassMode(bypass_mode::jpeg_compressed);

			// Default Background
            ///loadBackground();

			// Default Windowing Function
            ///setDispComp();

			// Default Clock Delay
			setClockDelay(CLOCK_GAIN * (double)m_pConfig->axsunClockDelay + CLOCK_OFFSET);

			// Default VDL Length
			m_pAxsunControl->setVDLHome();
			setVDLLength(m_pConfig->axsunVDLLength);

			// Default Contrast Range
			adjustDecibelRange();
        ///});
        ///setup.detach();
				
		// Set enabled true for Axsun OCT control widgets
		m_pToggleButton_LightSource->setEnabled(true);
		m_pToggleButton_LiveImaging->setEnabled(true);

        ///m_pLabel_Background->setEnabled(true);
        ///m_pPushButton_SetBackground->setEnabled(true);
        ///m_pPushButton_ResetBackground->setEnabled(true);
        ///m_pLabel_DispCompCoef->setEnabled(true);
        ///m_pLineEdit_a2->setEnabled(true);
        ///m_pLineEdit_a3->setEnabled(true);
        ///m_pPushButton_DispComp->setEnabled(true);

		m_pLabel_ClockDelay->setEnabled(true);
		m_pSpinBox_ClockDelay->setEnabled(true);
		m_pLabel_VDLLength->setEnabled(true);
		m_pSpinBox_VDLLength->setEnabled(true);
		m_pPushButton_VDLHome->setEnabled(true);

		m_pLabel_DecibelRange->setEnabled(true);
		m_pLineEdit_DecibelMax->setEnabled(true);
		m_pLineEdit_DecibelMin->setEnabled(true);
		m_pImageView_Colorbar->setEnabled(true);

		// Set buttons
		//m_pToggleButton_LightSource->setChecked(true); 
		//m_pToggleButton_LiveImaging->setChecked(true);
	}
	else
	{
		// Set disabled true for Axsun OCT control widgets
		m_pToggleButton_LightSource->setDisabled(true);
		m_pToggleButton_LiveImaging->setDisabled(true);

        ///m_pLabel_Background->setDisabled(true);
        ///m_pPushButton_SetBackground->setDisabled(true);
        ///m_pPushButton_ResetBackground->setDisabled(true);
        ///m_pLabel_DispCompCoef->setDisabled(true);
        ///m_pLineEdit_a2->setDisabled(true);
        ///m_pLineEdit_a3->setDisabled(true);
        ///m_pPushButton_DispComp->setDisabled(true);

		m_pLabel_ClockDelay->setDisabled(true);
		m_pSpinBox_ClockDelay->setDisabled(true);
		m_pLabel_VDLLength->setDisabled(true);
		m_pSpinBox_VDLLength->setDisabled(true);
		m_pPushButton_VDLHome->setDisabled(true);

		m_pLabel_DecibelRange->setDisabled(true);
		m_pLineEdit_DecibelMax->setDisabled(true);
		m_pLineEdit_DecibelMin->setDisabled(true);
		m_pImageView_Colorbar->setDisabled(true);
		
		// Set buttons
		if (m_pToggleButton_LightSource->isChecked()) m_pToggleButton_LightSource->setChecked(false);
		if (m_pToggleButton_LiveImaging->isChecked()) m_pToggleButton_LiveImaging->setChecked(false);

		if (m_pAxsunControl)
		{
			// Delete Axsun OCT control objects
			delete m_pAxsunControl;
			m_pAxsunControl = nullptr;
		}
		
		// Set text
		m_pCheckBox_AxsunOctControl->setText("Connect to Axsun OCT Control");
	}
}

void QDeviceControlTab::setLightSource(bool toggled)
{
	if (m_pAxsunControl)
	{
		if (toggled)
		{
			// Set text
			m_pToggleButton_LightSource->setText("Laser Off");

			// Start Axsun light source operation
			m_pAxsunControl->setLaserEmission(true);
		}
		else
		{
			// Stop Axsun light source operation
			m_pAxsunControl->setLaserEmission(false);

			// Set text
			m_pToggleButton_LightSource->setText("Laser On");
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
			m_pToggleButton_LiveImaging->setText("Imaging Off");
			if (!m_pStreamTab->getOperationTab()->isAcquisitionButtonToggled())
			{
				//m_pPushButton_SetBackground->setEnabled(true);
				//m_pPushButton_ResetBackground->setEnabled(true);
			}

			// Start Axsun live imaging operation
			m_pAxsunControl->setLiveImagingMode(true);
		}
		else
		{
			// Stop Axsun live imaging operation
			m_pAxsunControl->setLiveImagingMode(false);

			// Set text and widgets
			//m_pPushButton_SetBackground->setEnabled(false);
			//m_pPushButton_ResetBackground->setEnabled(false);
			m_pToggleButton_LiveImaging->setText("Imaging On");
		}
	}
}

void QDeviceControlTab::setBackground()
{
	if (m_pAxsunControl)
	{
		// BG capture mode
		m_pAxsunControl->setBypassMode(bypass_mode::abs2_with_bg_subtracted);

		// Reset BG FPGA
		m_pStreamTab->getOperationTab()->getProgressBar()->setRange(0, ALINE_LENGTH - 1); 
		m_pStreamTab->getOperationTab()->getProgressBar()->setValue(0);
		m_pStreamTab->getOperationTab()->getProgressBar()->setFormat("Resetting Axsun Background Array... %p%");

		m_pAxsunControl->setBackground(nullptr);

		m_pStreamTab->getOperationTab()->getProgressBar()->setFormat("");
		m_pStreamTab->getOperationTab()->getProgressBar()->setValue(0);
						
		// BG Capture and Transfer to relevant FPGA
		np::Uint8Array2 image_data_out;

		AxsunCapture* pAxsunCapture = m_pStreamTab->getOperationTab()->getDataAcq()->getAxsunCapture();		
		if (pAxsunCapture->requestBufferedImage(image_data_out))
		{
			// Stop Live Imaging
			m_pAxsunControl->setLiveImagingMode(false);

			// Get BG frame
			np::Uint16Array2 bg_frame(image_data_out.size(0) / sizeof(uint16_t), image_data_out.size(1));
			for (int i = 0; i < bg_frame.size(1); i++)
				for (int j = 0; j < bg_frame.size(0); j++)
					bg_frame(j, i) = ((uint16_t)image_data_out(2 * j, i) << 8) + (uint16_t)image_data_out(2 * j + 1, i);
			
			// Get averaged BG profile
			np::Uint16Array bg(bg_frame.size(0));
			for (int i = 0; i < bg.length(); i++)
			{
				double temp = 0;
				for (int j = 0; j < bg_frame.size(1); j++)
					temp += (double)bg_frame(i, j);
					
				bg(i) = (uint16_t)(temp / (double)bg_frame.size(1));
			}

			// Save BG profile
			QFile file("bg.bin");
			if (false != file.open(QIODevice::WriteOnly))
			{
				file.write(reinterpret_cast<char*>(bg.raw_ptr()), sizeof(uint16_t) * bg.length());
				file.close();
			}
			
			// Set BG FPGA
			m_pStreamTab->getOperationTab()->getProgressBar()->setRange(0, ALINE_LENGTH - 1);
			m_pStreamTab->getOperationTab()->getProgressBar()->setValue(0);
			m_pStreamTab->getOperationTab()->getProgressBar()->setFormat("Setting Axsun Background Array... %p%");

			m_pAxsunControl->setBackground(bg);

			m_pStreamTab->getOperationTab()->getProgressBar()->setFormat("");
			m_pStreamTab->getOperationTab()->getProgressBar()->setValue(0);
		}
		
		// JPEG mode
		m_pAxsunControl->setBypassMode(bypass_mode::jpeg_compressed);

		// Restart Live Imaging
		m_pAxsunControl->setLiveImagingMode(true);
	}
}

void QDeviceControlTab::resetBackground()
{
	if (m_pAxsunControl)
	{
		// Reset BG FPGA
		m_pStreamTab->getOperationTab()->getProgressBar()->setRange(0, ALINE_LENGTH - 1);
		m_pStreamTab->getOperationTab()->getProgressBar()->setValue(0);
		m_pStreamTab->getOperationTab()->getProgressBar()->setFormat("Resetting Axsun Background Array... %p%");

		m_pAxsunControl->setBackground(nullptr);

		m_pStreamTab->getOperationTab()->getProgressBar()->setFormat("");
		m_pStreamTab->getOperationTab()->getProgressBar()->setValue(0);
	}
}

void QDeviceControlTab::loadBackground()
{
	if (m_pAxsunControl)
	{
		np::Uint16Array bg(ALINE_LENGTH);

		// Load BG profile
		QFile file("bg.bin");
		if (false != file.open(QIODevice::ReadOnly))
		{
			file.read(reinterpret_cast<char*>(bg.raw_ptr()), sizeof(uint16_t) * bg.length());
			file.close();
		}

		// Set BG FPGA
		m_pStreamTab->getOperationTab()->getProgressBar()->setRange(0, ALINE_LENGTH - 1);
		m_pStreamTab->getOperationTab()->getProgressBar()->setValue(0);
		m_pStreamTab->getOperationTab()->getProgressBar()->setFormat("Setting Axsun Background Array... %p%");

		m_pAxsunControl->setBackground(bg);

		m_pStreamTab->getOperationTab()->getProgressBar()->setFormat("");
		m_pStreamTab->getOperationTab()->getProgressBar()->setValue(0);
	}
}

void QDeviceControlTab::setDispComp()
{
	if (m_pAxsunControl)
	{
		int a2 = m_pLineEdit_a2->text().toInt();
		int a3 = m_pLineEdit_a3->text().toInt();
		m_pConfig->axsunDisComA2 = a2;
		m_pConfig->axsunDisComA3 = a3;
		
		m_pStreamTab->getOperationTab()->getProgressBar()->setRange(0, 2 * MAX_SAMPLE_LENGTH - 1);
		m_pStreamTab->getOperationTab()->getProgressBar()->setValue(0);
		m_pStreamTab->getOperationTab()->getProgressBar()->setFormat("Setting Disp Comp Array... %p%");

		m_pAxsunControl->setDispersionCompensation(a2, a3);

		m_pStreamTab->getOperationTab()->getProgressBar()->setFormat("");
		m_pStreamTab->getOperationTab()->getProgressBar()->setValue(0);
	}
}

void QDeviceControlTab::setClockDelay(double delay)
{
	if (m_pAxsunControl)
	{
		unsigned long delay_level = (unsigned long)(round((delay - CLOCK_OFFSET) / CLOCK_GAIN));
		m_pConfig->axsunClockDelay = delay;

		m_pAxsunControl->setClockDelay(delay_level);
	}
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

void QDeviceControlTab::adjustDecibelRange()
{
	if (m_pAxsunControl)
	{
		double min = m_pLineEdit_DecibelMin->text().toDouble();
		double max = m_pLineEdit_DecibelMax->text().toDouble();

		m_pConfig->axsunDbRange.min = min;
		m_pConfig->axsunDbRange.max = max;

		m_pAxsunControl->setDecibelRange(min, max);
	}
}


void QDeviceControlTab::connectZaberStage(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_ZaberStageControl->setText("Disconnect from Zaber Stage");

        // Create Zaber stage control objects
		if (!m_pZaberStage)
		{
			m_pZaberStage = new ZaberStage;
			m_pZaberStage->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				qmsg.replace('\n', ' ');
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}

        // Connect stage
        if (!(m_pZaberStage->ConnectStage()))
        {
            m_pCheckBox_ZaberStageControl->setChecked(false);
            return;
        }

        // Set target speed first
        m_pZaberStage->SetTargetSpeed(m_pLineEdit_TargetSpeed->text().toDouble());

        // Set enable true for Zaber stage control widgets
        m_pPushButton_MoveAbsolute->setEnabled(true);
        m_pPushButton_SetTargetSpeed->setEnabled(true);
        m_pPushButton_Home->setEnabled(true);
        m_pPushButton_Stop->setEnabled(true);
        m_pLineEdit_TravelLength->setEnabled(true);
        m_pLineEdit_TargetSpeed->setEnabled(true);
        m_pLabel_TravelLength->setEnabled(true);
        m_pLabel_TargetSpeed->setEnabled(true);
    }
    else
    {
        // Set enable false for Zaber stage control widgets
        m_pPushButton_MoveAbsolute->setEnabled(false);
        m_pPushButton_SetTargetSpeed->setEnabled(false);
        m_pPushButton_Home->setEnabled(false);
        m_pPushButton_Stop->setEnabled(false);
        m_pLineEdit_TravelLength->setEnabled(false);
        m_pLineEdit_TargetSpeed->setEnabled(false);
        m_pLabel_TravelLength->setEnabled(false);
        m_pLabel_TargetSpeed->setEnabled(false);

        if (m_pZaberStage)
        {
            // Stop Wait Thread
            m_pZaberStage->StopWaitThread();

            // Disconnect the Stage
            m_pZaberStage->DisconnectStage();

            // Delete Zaber stage control objects
            delete m_pZaberStage;
            m_pZaberStage = nullptr;
        }

        // Set text
        m_pCheckBox_ZaberStageControl->setText("Connect to Zaber Stage");
    }
}

void QDeviceControlTab::moveAbsolute()
{
    m_pZaberStage->MoveAbsoulte(m_pLineEdit_TravelLength->text().toDouble());
}

void QDeviceControlTab::setTargetSpeed(const QString & str)
{
    m_pZaberStage->SetTargetSpeed(str.toDouble());
    m_pConfig->zaberPullbackSpeed = str.toInt();
}

void QDeviceControlTab::changeZaberPullbackLength(const QString &str)
{
    m_pConfig->zaberPullbackLength = str.toInt();
}

void QDeviceControlTab::home()
{
    m_pZaberStage->Home();
}

void QDeviceControlTab::stop()
{
    m_pZaberStage->Stop();
}


void QDeviceControlTab::connectFaulhaberMotor(bool toggled)
{
	if (toggled)
	{
		// Set text
        m_pCheckBox_FaulhaberMotorControl->setText("Disconnect from Faulhaber Motor");

		// Create Faulhaber motor control objects
		if (!m_pFaulhaberMotor)
		{
			m_pFaulhaberMotor = new FaulhaberMotor;
			m_pFaulhaberMotor->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				qmsg.replace('\n', ' ');
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}

		// Connect the motor
		if (!(m_pFaulhaberMotor->ConnectDevice()))
		{
			m_pCheckBox_FaulhaberMotorControl->setChecked(false);
			return;
		}
		
		// Set enable true for Faulhaber motor control widgets
		m_pToggleButton_Rotate->setEnabled(true);
		m_pLineEdit_RPM->setEnabled(true);
		m_pLabel_RPM->setEnabled(true);
	}
	else
	{
		// Set enable false for Faulhaber motor control widgets
		m_pToggleButton_Rotate->setChecked(false);
		m_pToggleButton_Rotate->setEnabled(false);
		m_pLineEdit_RPM->setEnabled(false);
		m_pLabel_RPM->setEnabled(false);
		
		if (m_pFaulhaberMotor)
		{
			// Disconnect the motor
			m_pFaulhaberMotor->DisconnectDevice();

			// Delete Faulhaber motor control objects
			delete m_pFaulhaberMotor;
            m_pFaulhaberMotor = nullptr;
		}

		// Set text
        m_pCheckBox_FaulhaberMotorControl->setText("Connect to Faulhaber Motor");
	}
}

void QDeviceControlTab::rotate(bool toggled)
{
	if (toggled)
	{
		m_pFaulhaberMotor->RotateMotor(m_pLineEdit_RPM->text().toInt());
		m_pLineEdit_RPM->setEnabled(false);
		m_pToggleButton_Rotate->setText("Stop");
	}
	else
	{
		m_pFaulhaberMotor->StopMotor();
		m_pLineEdit_RPM->setEnabled(true);
		m_pToggleButton_Rotate->setText("Rotate");
	}
}

void QDeviceControlTab::changeFaulhaberRpm(const QString &str)
{
	m_pConfig->faulhaberRpm = str.toInt();
}
