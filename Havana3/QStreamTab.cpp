
#include "QStreamTab.h"

#include <QSqlQuery>

#include <Havana3/MainWindow.h>
#include <Havana3/HvnSqlDataBase.h>
#include <Havana3/QViewTab.h>
#include <Havana3/Dialog/SettingDlg.h>
#include <Havana3/Dialog/FlimCalibTab.h>
#include <Havana3/Dialog/DeviceOptionTab.h>
#ifdef DEVELOPER_MODE
#include <Havana3/Viewer/QScope.h>
#endif

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/ThreadManager.h>
#ifndef NEXT_GEN_SYSTEM
#ifdef AXSUN_ENABLE
#include <DataAcquisition/AxsunCapture/AxsunCapture.h>
#endif
#include <DataAcquisition/SignatecDAQ/SignatecDAQ.h>
#else
#include <DataAcquisition/AlazarDAQ/AlazarDAQ.h>
#endif
#include <DataAcquisition/FLImProcess/FLImProcess.h>
#include <DataAcquisition/OCTProcess/OCTProcess.h>

#include <MemoryBuffer/MemoryBuffer.h>

#include <DeviceControl/DeviceControl.h>
#include <DeviceControl/FaulhaberMotor/RotaryMotor.h>
#include <DeviceControl/FaulhaberMotor/PullbackMotor.h>
#include <DeviceControl/ElforlightLaser/ElforlightLaser.h>
#ifdef AXSUN_ENABLE
#include <DeviceControl/AxsunControl/AxsunControl.h>
#endif

#include <iostream>
#include <thread>
#include <chrono>

#include <ippcore.h>
#include <ippi.h>
#include <ipps.h>


QStreamTab::QStreamTab(QString patient_id, QWidget *parent) :
    QDialog(parent), m_pSettingDlg(nullptr)
#ifdef DEVELOPER_MODE
	, m_pScope_Alines(nullptr)
#endif
{
	// Set main window objects
    m_pMainWnd = dynamic_cast<MainWindow*>(parent);
    m_pConfig = m_pMainWnd->m_pConfiguration;
    m_pHvnSqlDataBase = m_pMainWnd->m_pHvnSqlDataBase;

    // Create widgets for live streaming view
    createLiveStreamingViewWidgets();
	changePatient(patient_id);

    // Create data acquisition object
    m_pDataAcquisition = new DataAcquisition(m_pConfig);
	
    // Create thread managers for data processing
	m_pThreadFlimProcess = new ThreadManager("FLIm image process");
	m_pThreadOctProcess = new ThreadManager("OCT image process");
	m_pThreadVisualization = new ThreadManager("Visualization process");

	// Create buffers for threading operation
	m_syncFlimProcessing.allocate_queue_buffer(m_pConfig->flimScans, m_pConfig->flimAlines, PROCESSING_BUFFER_SIZE);  // FLIm Processing
#ifndef NEXT_GEN_SYSTEM
	if (m_pConfig->axsunPipelineMode == PipelineMode::JPEG_COMPRESSED)
		m_syncOctProcessing.allocate_queue_buffer(m_pConfig->octScans, m_pConfig->octAlines, PROCESSING_BUFFER_SIZE); // OCT Processing
	else
		m_syncOctProcessing.allocate_queue_buffer(4 * m_pConfig->octScans, m_pConfig->octAlines, PROCESSING_BUFFER_SIZE); // OCT Processing
#else
	m_syncOctProcessing.allocate_queue_buffer(m_pConfig->octScansFFT / 2, m_pConfig->octAlines, PROCESSING_BUFFER_SIZE); // OCT Processing
#endif
	m_syncFlimVisualization.allocate_queue_buffer(11, m_pConfig->flimAlines, PROCESSING_BUFFER_SIZE);  // FLIm Visualization
#ifndef NEXT_GEN_SYSTEM
	m_syncOctVisualization.allocate_queue_buffer(m_pConfig->octScans, m_pConfig->octAlines, PROCESSING_BUFFER_SIZE);  // OCT Visualization
#else
	m_syncOctVisualization.allocate_queue_buffer(m_pConfig->octScansFFT / 2, m_pConfig->octAlines, PROCESSING_BUFFER_SIZE);  // OCT Visualization
#endif

    // Set signal object
    setFlimAcquisitionCallback();
	setOctAcquisitionCallback();
    setFlimProcessingCallback();
	setOctProcessingCallback();
    setVisualizationCallback();

    // Create memory buffer object
    m_pMemoryBuffer = new MemoryBuffer(this);

    // Create device control object
    m_pDeviceControl = new DeviceControl(m_pConfig);
	
    // Set window layout
    QHBoxLayout* pHBoxLayout = new QHBoxLayout;
    pHBoxLayout->setSpacing(2);

    pHBoxLayout->addWidget(m_pGroupBox_LiveStreaming);

    this->setLayout(pHBoxLayout);
}

QStreamTab::~QStreamTab()
{
#ifdef DEVELOPER_MODE
	// Stop timers
	m_pSyncMonitorTimer->stop();
	m_pLaserMonitorTimer->stop();
#endif

	// Delete relevant objects
	if (m_pDeviceControl) delete m_pDeviceControl;
	if (m_pDataAcquisition) delete m_pDataAcquisition;

	// Delete threading objects
	if (m_pThreadVisualization) delete m_pThreadVisualization;
	if (m_pThreadOctProcess) delete m_pThreadOctProcess;
	if (m_pThreadFlimProcess) delete m_pThreadFlimProcess;
	
	// Disallocate buffers for threading operation
	m_syncFlimProcessing.deallocate_queue_buffer();
	m_syncOctProcessing.deallocate_queue_buffer();
	m_syncFlimVisualization.deallocate_queue_buffer();
	m_syncOctVisualization.deallocate_queue_buffer();

#ifdef DEVELOPER_MODE
	if (m_pScope_Alines)
	{
		///m_pScope_Alines->close();
		///m_pScope_Alines->deleteLater();
	}
#endif
}


void QStreamTab::closeEvent(QCloseEvent *e)
{
	if (m_pSettingDlg)
		m_pSettingDlg->close();

	e->accept();
}

void QStreamTab::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
}


void QStreamTab::createLiveStreamingViewWidgets()
{
    // Create widgets for live streaming view
    m_pGroupBox_LiveStreaming = new QGroupBox(this);
    m_pGroupBox_LiveStreaming->setFixedWidth(850);
    m_pGroupBox_LiveStreaming->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    m_pGroupBox_LiveStreaming->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    m_pLabel_PatientInformation = new QLabel(this);
    m_pLabel_PatientInformation->setStyleSheet("QLabel{font-size:12pt}");

    m_pViewTab = new QViewTab(true, this);
	 
	//m_pToggleButton_CatheterConnection = new QPushButton(this);
	//m_pToggleButton_CatheterConnection->setText("Connection");
	//m_pToggleButton_CatheterConnection->setCheckable(true);
	//m_pToggleButton_CatheterConnection->setFixedSize(70, 32);

	// Hellical scanning
    m_pToggleButton_EnableRotation = new QPushButton(this);
    m_pToggleButton_EnableRotation->setText(" Enable Rotation");
    m_pToggleButton_EnableRotation->setCheckable(true);
    m_pToggleButton_EnableRotation->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_pToggleButton_EnableRotation->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#00ff00}");
    m_pToggleButton_EnableRotation->setFixedSize(180, 35);

    m_pToggleButton_StartPullback = new QPushButton(this);
    m_pToggleButton_StartPullback->setText(" Start Pullback");
    m_pToggleButton_StartPullback->setCheckable(true);
    m_pToggleButton_StartPullback->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    m_pToggleButton_StartPullback->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#00ff00}");
    m_pToggleButton_StartPullback->setFixedSize(180, 35);
	m_pToggleButton_StartPullback->setDisabled(true);
	
	// Catheter calibration
	m_pLabel_CatheterCalibration = new QLabel(this);
	m_pLabel_CatheterCalibration->setText("Catheter\nCalibration");
	m_pLabel_CatheterCalibration->setAlignment(Qt::AlignCenter);

	m_pPushButton_CatheterCalibrationReset = new QPushButton(this);
	m_pPushButton_CatheterCalibrationReset->setText("Reset");
	m_pPushButton_CatheterCalibrationReset->setFixedSize(40, 32);

	m_pScrollBar_CatheterCalibration = new QScrollBar(this);
	m_pScrollBar_CatheterCalibration->setFixedSize(180, 16);
	m_pScrollBar_CatheterCalibration->setOrientation(Qt::Horizontal);
	m_pScrollBar_CatheterCalibration->setRange(0, 1500);
	m_pScrollBar_CatheterCalibration->setSingleStep(1);
	m_pScrollBar_CatheterCalibration->setPageStep(10);
	m_pScrollBar_CatheterCalibration->setValue((int)(m_pConfig->axsunVDLLength * 100.0f));

	m_pScrollBar_InnerOffset = new QScrollBar(this);
	m_pScrollBar_InnerOffset->setFixedSize(180, 16);
	m_pScrollBar_InnerOffset->setOrientation(Qt::Horizontal);
	m_pScrollBar_InnerOffset->setRange(0, 512);
	m_pScrollBar_InnerOffset->setSingleStep(1);
	m_pScrollBar_InnerOffset->setPageStep(10);
	m_pScrollBar_InnerOffset->setValue(m_pConfig->innerOffsetLength);

	// Setting window
    m_pPushButton_Setting = new QPushButton(this);
    m_pPushButton_Setting->setText("  Setting");
    m_pPushButton_Setting->setIcon(style()->standardIcon(QStyle::SP_FileDialogInfoView));
    m_pPushButton_Setting->setFixedSize(100, 25);

#ifdef DEVELOPER_MODE
#ifndef NEXT_GEN_SYSTEM
	///m_pScope_Alines = new QScope({ 0, (double)m_pConfig->octScans }, { 0.0, 255.0 }, 2, 2, 1, 1, 0, 0, "", "", false, false, this);
#else
	///m_pScope_Alines = new QScope({ 0, (double)m_pConfig->octScansFFT / 2.0 }, { m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max }, 2, 2, 1, 1, 0, 0, "", "dB", false, false, this);
#endif
	///m_pScope_Alines->setWindowTitle("OCT A-lines");
	///m_pScope_Alines->setFixedSize(640, 240);
	///connect(this, SIGNAL(plotAline(const float*)), m_pScope_Alines, SLOT(drawData(const float*)));
	///m_pScope_Alines->show();

	m_pLabel_StreamingSyncStatus = new QLabel(this);
	m_pLabel_StreamingSyncStatus->setFixedSize(150, 200);
	m_pLabel_StreamingSyncStatus->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	m_pLabel_StreamingSyncStatus->setStyleSheet("QLabel{font-size:10; color:#a0a0a0}");

	m_pLabel_LaserStatus = new QLabel(this);
	m_pLabel_LaserStatus->setFixedSize(200, 220);
	m_pLabel_LaserStatus->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
	m_pLabel_LaserStatus->setStyleSheet("QLabel{font-size:10; color:#a0a0a0}");
	m_pLabel_LaserStatus->setText(QString::fromLocal8Bit("[FLIm Laser Set/Monitor]\nDiode Current: 0.00 / 0.00 A\nDiode Temp: 0.00 / 0.00 ��C\nChipset Temp: 0.00 / 0.00 ��C"));
	
	m_pSyncMonitorTimer = new QTimer(this);
	m_pSyncMonitorTimer->start(1000);
	connect(m_pSyncMonitorTimer, SIGNAL(timeout()), this, SLOT(onTimerSyncMonitor()));

	m_pLaserMonitorTimer = new QTimer(this);
	m_pLaserMonitorTimer->start(60000);
	connect(m_pLaserMonitorTimer, SIGNAL(timeout()), this, SLOT(onTimerLaserMonitor()));
#endif

    // Set layout: live streaming view
    QVBoxLayout *pVBoxLayout_LiveStreaming = new QVBoxLayout;
    pVBoxLayout_LiveStreaming->setSpacing(0);
    pVBoxLayout_LiveStreaming->setAlignment(Qt::AlignCenter);

    QHBoxLayout *pHBoxLayout_Title = new QHBoxLayout;
    pHBoxLayout_Title->setSpacing(5);
    pHBoxLayout_Title->setAlignment(Qt::AlignCenter);
    pHBoxLayout_Title->addWidget(m_pLabel_PatientInformation);
    pHBoxLayout_Title->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_Title->addWidget(m_pPushButton_Setting);

    QGridLayout *pGridLayout_Operation = new QGridLayout;
	pGridLayout_Operation->setSpacing(5);
	pGridLayout_Operation->setAlignment(Qt::AlignVCenter);
	pGridLayout_Operation->addWidget(m_pLabel_CatheterCalibration, 0, 0, 2, 1);
	pGridLayout_Operation->addWidget(m_pPushButton_CatheterCalibrationReset, 0, 1, 2, 1);
	pGridLayout_Operation->addWidget(m_pScrollBar_CatheterCalibration, 0, 2, Qt::AlignTop);
	pGridLayout_Operation->addWidget(m_pScrollBar_InnerOffset, 1, 2, Qt::AlignBottom);
	pGridLayout_Operation->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 3, 2, 1);
	pGridLayout_Operation->addWidget(m_pToggleButton_EnableRotation, 0, 4, 2, 1);
	pGridLayout_Operation->addWidget(m_pToggleButton_StartPullback, 0, 5, 2, 1);

    pVBoxLayout_LiveStreaming->addItem(pHBoxLayout_Title);
    pVBoxLayout_LiveStreaming->addWidget(m_pViewTab->getViewWidget());
    pVBoxLayout_LiveStreaming->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
    pVBoxLayout_LiveStreaming->addItem(pGridLayout_Operation);

    m_pGroupBox_LiveStreaming->setLayout(pVBoxLayout_LiveStreaming);

    // Connect signal and slot
	connect(this, SIGNAL(getCapture(QByteArray &)), m_pViewTab, SLOT(getCapture(QByteArray &)));
	connect(m_pPushButton_CatheterCalibrationReset, SIGNAL(clicked(bool)), this, SLOT(resetCatheterCalibration()));
    connect(m_pScrollBar_CatheterCalibration, SIGNAL(valueChanged(int)), this, SLOT(scrollCatheterCalibration(int)));
	connect(m_pScrollBar_InnerOffset, SIGNAL(valueChanged(int)), this, SLOT(scrollInnerOffsetLength(int)));
	///connect(m_pToggleButton_CatheterConnection, SIGNAL(toggled(bool)), this, SLOT(catheterConnection(bool)));
    connect(m_pToggleButton_EnableRotation, SIGNAL(toggled(bool)), this, SLOT(enableRotation(bool)));
    connect(m_pToggleButton_StartPullback, SIGNAL(toggled(bool)), this, SLOT(startPullback(bool)));
    connect(m_pPushButton_Setting, SIGNAL(clicked(bool)), this, SLOT(createSettingDlg()));
	connect(this, SIGNAL(pullbackFinished(bool)), m_pToggleButton_StartPullback, SLOT(setChecked(bool)));
#ifdef DEVELOPER_MODE
	connect(this, SIGNAL(setStreamingSyncStatusLabel(const QString &)), m_pLabel_StreamingSyncStatus, SLOT(setText(const QString &)));
	connect(this, SIGNAL(setLaserStatusLabel(const QString &)), m_pLabel_LaserStatus, SLOT(setText(const QString &)));
#endif
}


void QStreamTab::changePatient(QString patient_id)
{
	m_recordInfo.patientId = patient_id;

	QString command = QString("SELECT * FROM patients WHERE patient_id=%1").arg(m_recordInfo.patientId);

	m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {
		while (_sqlQuery.next())
		{
			m_recordInfo.patientName = _sqlQuery.value(1).toString() + ", " + _sqlQuery.value(0).toString();

			QDate date_of_birth = QDate::fromString(_sqlQuery.value(4).toString(), "yyyy-MM-dd");
			QDate date_today = QDate::currentDate();
			int age = (int)((double)date_of_birth.daysTo(date_today) / 365.25);
			QString gender = m_pHvnSqlDataBase->getGender(_sqlQuery.value(5).toInt());

			m_pLabel_PatientInformation->setText(QString("<b>%1 (%2%3)</b>"
				"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>ID:</b> %4").
				arg(m_recordInfo.patientName).arg(age).arg(gender).
				arg(QString("%1").arg(m_recordInfo.patientId.toInt(), 8, 10, QChar('0'))));
			QString title = QString("Live Streaming: %1").arg(m_recordInfo.patientName);
			setWindowTitle(title);
			
			// Set tab titles
			int index = 0;
			foreach(QDialog* pTabView, m_pMainWnd->getVectorTabViews())
			{
				if (pTabView->windowTitle().contains("Streaming"))
					m_pMainWnd->getTabWidget()->setTabText(index, this->windowTitle());
				index++;
			}
		}

		m_pConfig->writeToLog(QString("Live streaming patient changed: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));
	});
}


void QStreamTab::startLiveImaging(bool start)
{
	if (start)
	{	
		// Show message box
		QMessageBox msg_box(QMessageBox::NoIcon, "Device Initialization...", "", QMessageBox::NoButton, this);
		msg_box.setStandardButtons(0);
		msg_box.setWindowModality(Qt::WindowModal);
		msg_box.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
		msg_box.move(QApplication::desktop()->screen()->rect().center() - msg_box.rect().center());
		msg_box.setFixedSize(msg_box.width(), msg_box.height());
		connect(this, SIGNAL(deviceInitialized()), &msg_box, SLOT(accept()));
		msg_box.show();

		// Enable memory, data acq, device control			
		if (!enableMemoryBuffer(true) || !enableDataAcquisition(true) || !enableDeviceControl(true))
		{
			startLiveImaging(false);
			m_pConfig->writeToLog(QString("Live streaming failed: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));

			return;
		}

		emit deviceInitialized();

		m_pToggleButton_EnableRotation->setChecked(true);
		m_pScrollBar_CatheterCalibration->setValue((int)(m_pConfig->axsunVDLLength * 100.0f));
		
		m_pConfig->writeToLog(QString("Live streaming started: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));
	}
	else
	{		
		if (m_pToggleButton_EnableRotation->isChecked())
			m_pToggleButton_EnableRotation->setChecked(false);

		enableDeviceControl(false);
		enableDataAcquisition(false);
		enableMemoryBuffer(false);

		m_pConfig->writeToLog(QString("Live streaming stopped: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));
	}
}


bool QStreamTab::enableMemoryBuffer(bool enabled)
{
	if (enabled)
	{
		// Create buffers for data recording
        std::thread allocate_buffers([&]() {        
            m_pMemoryBuffer->allocateWritingBuffer();
		});
        allocate_buffers.detach();
	}
	else
    {
		// Disallocate buffers for data recording
		m_pMemoryBuffer->disallocateWritingBuffer();
    }

	return true;
}

bool QStreamTab::enableDataAcquisition(bool enabled)
{
    if (enabled) // Enable data acquisition object
    {		
        if (m_pDataAcquisition->InitializeAcquistion())
        {
            // Start thread process
            m_pThreadVisualization->startThreading();
			m_pThreadOctProcess->startThreading();
            m_pThreadFlimProcess->startThreading();

            // Start data acquisition			
			if (m_pDataAcquisition->StartAcquisition())
				return true;
			else
				return false;
        }
		return false;
    }
	else
	{
		// Disable data acquisition object
		m_pDataAcquisition->StopAcquisition();

		m_pThreadFlimProcess->stopThreading();
		m_pThreadOctProcess->stopThreading();
		m_pThreadVisualization->stopThreading();

		return true;
	}
}

bool QStreamTab::enableDeviceControl(bool enabled)
{
	if (enabled)
	{		
		// Set rotary & pullback motor control
		if (!m_pDeviceControl->connectRotaryMotor(true)) return false;		
		if (!m_pDeviceControl->connectPullbackMotor(true))
			return false;
		else
		{
			m_pDeviceControl->getPullbackMotor()->DidRotateEnd.clear();
			m_pDeviceControl->getPullbackMotor()->DidRotateEnd += [&](int timeout)
			{
				if (timeout)
				{
					m_pConfig->pullbackFlag = false;
					m_pConfig->setConfigFile("Havana3.ini");
				}
			};
			if (m_pConfig->autoHomeMode) m_pDeviceControl->home();
		}

		// Set FLIm system control
		if (!m_pDeviceControl->connectFlimLaser(true)) 
			return false;
		else
		{
			m_pDeviceControl->getElforlightLaser()->UpdateState += [&](double* value) {
				emit setLaserStatusLabel(QString::fromLocal8Bit("[FLIm Laser Set/Monitor]\nDiode Current: %1 / %2 A\nDiode Temp: %3 / %4 ��C\nChipset Temp: %5 / %6 ��C")
						.arg(value[0], 3, 'f', 2).arg(value[1], 3, 'f', 2).arg(value[2], 3, 'f', 2)
						.arg(value[3], 3, 'f', 2).arg(value[4], 3, 'f', 2).arg(value[5], 3, 'f', 2));
			};
		}
#ifdef NEXT_GEN_SYSTEM
		else
		{
			m_pDeviceControl->adjustLaserPower(m_pConfig->flimLaserPower);
			m_pDeviceControl->monitorLaserStatus();
		}
#endif		
		if (!m_pDeviceControl->applyPmtGainVoltage(true)) return false;
		
		// Set master synchronization control
		if (!m_pDeviceControl->startSynchronization(true)) return false;
		
		// Set OCT system control
		if (!m_pDeviceControl->connectAxsunControl(true)) 
			return false;
		else
		{
			// Set master trigger generation + Axsun imaging mode on
			std::this_thread::sleep_for(std::chrono::milliseconds(500));			
#ifndef NEXT_GEN_SYSTEM
			//m_pDeviceControl->setSubSampling(1);
			m_pDeviceControl->setLiveImaging(true);
#endif
			m_pDeviceControl->setLightSource(true);
#ifndef NEXT_GEN_SYSTEM
			//m_pDeviceControl->setVDLLength(m_pConfig->axsunVDLLength);
#endif
		}
	
#ifdef DEVELOPER_MODE
	///m_pScope_Alines->raise();
	///m_pScope_Alines->activateWindow();
#endif
	}
	else
	{
		// Turn off all devices
		m_pDeviceControl->turnOffAllDevices();
	}

    return true;
}


void QStreamTab::setFlimAcquisitionCallback()
{
#ifndef NEXT_GEN_SYSTEM
    m_pDataAcquisition->ConnectAcquiredFlimData([&](int frame_count, const np::Array<uint16_t, 2>& frame) {
#else
	m_pDataAcquisition->ConnectAcquiredFlimData1([&](int frame_count, const void* _frame_ptr) {
#endif

        // Data transfer
		int renewal_count = RENEWAL_COUNT /
			(m_pToggleButton_EnableRotation->isChecked() &&
				!m_pToggleButton_StartPullback->isChecked() &&
				m_pConfig->axsunPipelineMode == 0 ? REDUCED_COUNT : 1);
        if (!(frame_count % renewal_count))
        {
            // Data transfer for FLIm processing
#ifndef NEXT_GEN_SYSTEM
            const uint16_t* frame_ptr = frame.raw_ptr();
#else
			const uint16_t* frame_ptr = (uint16_t*)_frame_ptr;
#endif

            // Get buffer from threading queue
            uint16_t* pulse_ptr = nullptr;
            {
                std::unique_lock<std::mutex> lock(m_syncFlimProcessing.mtx);

                if (!m_syncFlimProcessing.queue_buffer.empty())
                {
                    pulse_ptr = m_syncFlimProcessing.queue_buffer.front();
                    m_syncFlimProcessing.queue_buffer.pop();
                }
            }

            if (pulse_ptr != nullptr)
            {
                // Body
                memcpy(pulse_ptr, frame_ptr, sizeof(uint16_t) * m_pConfig->flimFrameSize);

                // Push the buffer to sync Queue
                m_syncFlimProcessing.Queue_sync.push(pulse_ptr);
				//m_syncFlimProcessing.n_exec++;
            }
        }

        // Buffering (When recording)
        if (m_pMemoryBuffer->m_bIsRecording)
		{
            if (m_pMemoryBuffer->m_nRecordedFrames < WRITING_BUFFER_SIZE)
			{
				// Get buffer from writing queue
				uint16_t* pulse_ptr = nullptr;
				{
                    std::unique_lock<std::mutex> lock(m_pMemoryBuffer->m_syncFlimBuffering.mtx);

                    if (!m_pMemoryBuffer->m_syncFlimBuffering.queue_buffer.empty())
					{
                        pulse_ptr = m_pMemoryBuffer->m_syncFlimBuffering.queue_buffer.front();
                        m_pMemoryBuffer->m_syncFlimBuffering.queue_buffer.pop();
					}
				}

				if (pulse_ptr != nullptr)
				{
					// Body (Copying the frame data)
#ifndef NEXT_GEN_SYSTEM
					memcpy(pulse_ptr, frame.raw_ptr(), sizeof(uint16_t) * m_pConfig->flimFrameSize);
#else
					memcpy(pulse_ptr, (uint16_t*)_frame_ptr, sizeof(uint16_t) * m_pConfig->flimFrameSize);
#endif

					// Push to the copy queue for copying transfered data in copy thread
                    m_pMemoryBuffer->m_syncFlimBuffering.Queue_sync.push(pulse_ptr);
				}
			}
			else
			{
				// Finish recording when the buffer is full
				m_pMemoryBuffer->stopRecording();
			}
		}
    });

    m_pDataAcquisition->ConnectStopFlimData([&]() {
        m_syncFlimProcessing.Queue_sync.push(nullptr);
    });

    m_pDataAcquisition->ConnectFlimSendStatusMessage([&](const char * msg, bool is_error) {
		if (is_error)
		{
			if (m_pDataAcquisition->getAcquisitionState())
			{
				enableDeviceControl(false);
				enableDataAcquisition(false);
			}
		}
		(void)msg;
    });
}

void QStreamTab::setOctAcquisitionCallback()
{
#ifndef NEXT_GEN_SYSTEM
	m_pDataAcquisition->ConnectAcquiredOctData([&](uint32_t frame_count, const np::Uint8Array2& frame) {
#else
	m_pDataAcquisition->ConnectAcquiredOctData1([&](int frame_count, const void* _frame_ptr) {
#endif
		// Data transfer
		int renewal_count = RENEWAL_COUNT / 
			(m_pToggleButton_EnableRotation->isChecked() && 
				!m_pToggleButton_StartPullback->isChecked() && 
				m_pConfig->axsunPipelineMode == 0 ? REDUCED_COUNT : 1);
		if (!(frame_count % renewal_count))
		{
#ifndef NEXT_GEN_SYSTEM
			// Data transfer for OCT processing
			const uint8_t* frame_ptr = frame.raw_ptr();

			// Get buffer from threading queue
			uint8_t* oct_ptr = nullptr;
#else
			// Data transfer for OCT processing
			const float* frame_ptr = (float*)_frame_ptr;

			// Get buffer from threading queue
			float* oct_ptr = nullptr;
#endif
			{
				std::unique_lock<std::mutex> lock(m_syncOctProcessing.mtx);

				if (!m_syncOctProcessing.queue_buffer.empty())
				{
					oct_ptr = m_syncOctProcessing.queue_buffer.front();
					m_syncOctProcessing.queue_buffer.pop();
				}
			}

			if (oct_ptr != nullptr)
			{
				// Body
#ifndef NEXT_GEN_SYSTEM
				if (m_pConfig->axsunPipelineMode == PipelineMode::JPEG_COMPRESSED)
					memcpy(oct_ptr, frame_ptr, sizeof(uint8_t) * m_pConfig->octFrameSize);
				else
					memcpy(oct_ptr, frame_ptr, sizeof(uint8_t) * 4 * m_pConfig->octFrameSize);
#else
				memcpy(oct_ptr, frame_ptr, sizeof(float) * m_pConfig->octFrameSize);
#endif

				// Push the buffer to sync Queue
				m_syncOctProcessing.Queue_sync.push(oct_ptr);
				///m_syncOctProcessing.n_exec++;
			}
		}

		// Buffering (When recording)
		if (m_pMemoryBuffer->m_bIsRecording)
		{
			if (m_pMemoryBuffer->m_nRecordedFrames < WRITING_BUFFER_SIZE)
			{
				// Get buffer from writing queue
#ifndef NEXT_GEN_SYSTEM
				uint8_t* oct_ptr = nullptr;
#else
				float* oct_ptr = nullptr;
#endif
				{
					std::unique_lock<std::mutex> lock(m_pMemoryBuffer->m_syncOctBuffering.mtx);

					if (!m_pMemoryBuffer->m_syncOctBuffering.queue_buffer.empty())
					{
						oct_ptr = m_pMemoryBuffer->m_syncOctBuffering.queue_buffer.front();
						m_pMemoryBuffer->m_syncOctBuffering.queue_buffer.pop();
					}
				}

				if (oct_ptr != nullptr)
				{
					// Body (Copying the frame data)
					const uint8_t* frame_ptr = frame.raw_ptr();
#ifndef NEXT_GEN_SYSTEM
					if (m_pConfig->axsunPipelineMode == PipelineMode::JPEG_COMPRESSED)
						memcpy(oct_ptr, frame_ptr, sizeof(uint8_t) * m_pConfig->octFrameSize);
					else
						memcpy(oct_ptr, frame_ptr, sizeof(uint8_t) * 4 * m_pConfig->octFrameSize);
#else
					memcpy(oct_ptr, (float*)_frame_ptr, sizeof(float) * m_pConfig->octFrameSize);
#endif

					// Push to the copy queue for copying transfered data in copy thread
					m_pMemoryBuffer->m_syncOctBuffering.Queue_sync.push(oct_ptr);
				}
			}
		}
	});

	m_pDataAcquisition->ConnectAcquiredOctBG([&](uint32_t frame_count, const np::Uint8Array2& frame) {
#ifdef AXSUN_ENABLE
		np::Uint16Array2 bg((uint16_t*)frame.raw_ptr(), frame.size(0) / 2, frame.size(1));
		ippiTranspose_16u_C1R(bg, sizeof(uint16_t) * bg.size(0),
			getDeviceControl()->getAxsunControl()->background_frame, sizeof(uint16_t) * bg.size(1), { bg.size(0), bg.size(1) });

		//QFile file("oct.data");
		//if (file.open(QIODevice::WriteOnly))
		//	file.write(reinterpret_cast<const char*>(getDeviceControl()->getAxsunControl()->background_frame.raw_ptr()), 
		//		sizeof(uint16_t) * getDeviceControl()->getAxsunControl()->background_frame.length());
		//file.close();
#else		
		(void)frame;
#endif
		(void)frame_count;
	});

	m_pDataAcquisition->ConnectStopOctData([&]() {
		m_syncOctProcessing.Queue_sync.push(nullptr);
	});

	m_pDataAcquisition->ConnectFlimSendStatusMessage([&](const char * msg, bool is_error) {
		if (is_error)
		{
			if (m_pDataAcquisition->getAcquisitionState())
			{
				enableDeviceControl(false);
				enableDataAcquisition(false);
			}
		}
		(void)msg;
	});
}

void QStreamTab::setFlimProcessingCallback()
{
    // FLIm Process Signal Objects /////////////////////////////////////////////////////////////////////////////////////////
    FLImProcess *pFLIm = m_pDataAcquisition->getFLIm();
    m_pThreadFlimProcess->DidAcquireData += [&, pFLIm](int frame_count) {

        // Get the buffer from the previous sync Queue
        uint16_t* pulse_data = m_syncFlimProcessing.Queue_sync.pop();
        if (pulse_data != nullptr)
        {
            // Get buffers from threading queues
            float* flim_ptr = nullptr;
            {
                std::unique_lock<std::mutex> lock(m_syncFlimVisualization.mtx);

                if (!m_syncFlimVisualization.queue_buffer.empty())
                {
                    flim_ptr = m_syncFlimVisualization.queue_buffer.front();
                    m_syncFlimVisualization.queue_buffer.pop();
                }
            }

            if (flim_ptr != nullptr)
            {
                //std::chrono::system_clock::time_point StartTime = std::chrono::system_clock::now();

                // Body
                np::FloatArray2 intensity(flim_ptr + 0 * m_pConfig->flimAlines, m_pConfig->flimAlines, 4);
                np::FloatArray2 mean_delay(flim_ptr + 4 * m_pConfig->flimAlines, m_pConfig->flimAlines, 4);
                np::FloatArray2 lifetime(flim_ptr + 8 * m_pConfig->flimAlines, m_pConfig->flimAlines, 3);
                np::Uint16Array2 pulse(pulse_data, m_pConfig->flimScans, m_pConfig->flimAlines);

                (*pFLIm)(intensity, mean_delay, lifetime, pulse);

				///if (INTRA_FRAME_SYNC > 0)
				///{
				///	for (int i = 0; i < 11; i++)
				///	{
				///		float* pValue = flim_ptr + i * m_pConfig->flimAlines;
				///		std::rotate(pValue, pValue + INTRA_FRAME_SYNC, pValue + m_pConfig->flimAlines);
				///	}
				///}

                // Transfer to FLIm calibration dlg
				if (m_pSettingDlg)
				{
					if (m_pSettingDlg->getTabWidget()->currentIndex() == 2) // FLIm calibration view
						emit m_pSettingDlg->getFlimCalibTab()->plotRoiPulse(0);
				}
				
                // Push the buffers to sync Queues
                m_syncFlimVisualization.Queue_sync.push(flim_ptr);
                ///m_syncVisualization.n_exec++;

                // Return (push) the buffer to the previous threading queue
                {
                    std::unique_lock<std::mutex> lock(m_syncFlimProcessing.mtx);
                    m_syncFlimProcessing.queue_buffer.push(pulse_data);
                }
            }
        }
        else
            m_pThreadFlimProcess->_running = false;

		(void)frame_count;
    };

    m_pThreadFlimProcess->DidStopData += [&]() {
        m_syncFlimVisualization.Queue_sync.push(nullptr);
    };

    m_pThreadFlimProcess->SendStatusMessage += [&](const char* msg, bool is_error) {
		
		QString qmsg = QString::fromUtf8(msg);
		if (is_error)
		{
			m_pConfig->writeToLog(QString("[ERROR] %1").arg(qmsg));
			QMessageBox MsgBox(QMessageBox::Critical, "Thread Mananger Error", qmsg);
			MsgBox.exec();

			if (m_pDataAcquisition->getAcquisitionState())
			{
				enableDeviceControl(false);
				enableDataAcquisition(false);
			}
		}
		else
		{
			m_pConfig->writeToLog(qmsg);
			qDebug() << qmsg;
		}
    };
}

void QStreamTab::setOctProcessingCallback()
{
	// OCT Process Signal Objects /////////////////////////////////////////////////////////////////////////////////////////	
	OCTProcess *pOCT = m_pDataAcquisition->getOCT();
	m_pThreadOctProcess->DidAcquireData += [&, pOCT](int frame_count) {

		// Get the buffer from the previous sync Queue
#ifndef NEXT_GEN_SYSTEM
		uint8_t* oct_data = m_syncOctProcessing.Queue_sync.pop();
#else
		float* oct_data = m_syncOctProcessing.Queue_sync.pop();
#endif
		if (oct_data != nullptr)
		{
			// Get buffers from threading queues
			uint8_t* img_ptr = nullptr;
			{
				std::unique_lock<std::mutex> lock(m_syncOctVisualization.mtx);

				if (!m_syncOctVisualization.queue_buffer.empty())
				{
					img_ptr = m_syncOctVisualization.queue_buffer.front();
					m_syncOctVisualization.queue_buffer.pop();
				}
			}

			if (img_ptr != nullptr)
			{
				//std::chrono::system_clock::time_point StartTime = std::chrono::system_clock::now();

				// Body
#ifndef NEXT_GEN_SYSTEM
				if (m_pConfig->axsunPipelineMode == PipelineMode::JPEG_COMPRESSED)
					memcpy(img_ptr, oct_data, sizeof(uint8_t) * m_pConfig->octFrameSize);
				else
					(*pOCT)(img_ptr, (int16_t*)oct_data, m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max);
#else
				ippiScale_32f8u_C1R(oct_data, sizeof(float) * m_pConfig->octScansFFT / 2,
					img_ptr, sizeof(uint8_t) * m_pConfig->octScansFFT / 2,
					{ m_pConfig->octScansFFT / 2, m_pConfig->octAlines }, m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max);
#endif

#ifdef DEVELOPER_MODE
				// Transfer to OCT A-line scope
				if (m_pScope_Alines)
				{
#ifndef NEXT_GEN_SYSTEM
					///np::FloatArray aline(m_pConfig->octScans);
					///ippsConvert_8u32f(img_ptr, aline, m_pConfig->octScans);
					///emit plotAline(aline);
#else
					emit plotAline(oct_data);
#endif
				}
#endif
				
				// Push the buffers to sync Queues
				m_syncOctVisualization.Queue_sync.push(img_ptr);
				///m_syncOctVisualization.n_exec++;

				// Return (push) the buffer to the previous threading queue
				{
					std::unique_lock<std::mutex> lock(m_syncOctProcessing.mtx);
					m_syncOctProcessing.queue_buffer.push(oct_data);
				}
			}
		}
		else
			m_pThreadOctProcess->_running = false;

		(void)frame_count;
	};

	m_pThreadOctProcess->DidStopData += [&]() {
		m_syncOctVisualization.Queue_sync.push(nullptr);
	};

	m_pThreadOctProcess->SendStatusMessage += [&](const char* msg, bool is_error) {

		QString qmsg = QString::fromUtf8(msg);
		if (is_error)
		{
			m_pConfig->writeToLog(QString("[ERROR] %1").arg(qmsg));
			QMessageBox MsgBox(QMessageBox::Critical, "Thread Mananger Error", qmsg);
			MsgBox.exec();

			if (m_pDataAcquisition->getAcquisitionState())
			{
				enableDeviceControl(false);
				enableDataAcquisition(false);
			}
		}
		else
		{
			m_pConfig->writeToLog(qmsg);
			qDebug() << qmsg;
		}
	};
}

void QStreamTab::setVisualizationCallback()
{
    // Visualization Signal Objects ///////////////////////////////////////////////////////////////////////////////////////////
#ifndef NEXT_GEN_SYSTEM
	m_pViewTab->m_visImage = np::Uint8Array2(m_pConfig->octScans, m_pConfig->octAlines);
#else
	m_pViewTab->m_visImage = np::Uint8Array2(m_pConfig->octScansFFT / 2, m_pConfig->octAlines);
#endif
	m_pViewTab->m_visIntensity = np::FloatArray2(m_pConfig->flimAlines, 4);
	m_pViewTab->m_visLifetime = np::FloatArray2(m_pConfig->flimAlines, 3);
	
    m_pThreadVisualization->DidAcquireData += [&](int frame_count) {
		
        // Get the buffers from the previous sync Queues
        float* flim_data = m_syncFlimVisualization.Queue_sync.pop();
        uint8_t* oct_data = m_syncOctVisualization.Queue_sync.pop();
        if ((flim_data != nullptr) && (oct_data != nullptr)) 
        {
            // Body
            if (m_pDataAcquisition->getAcquisitionState()) // Only valid if acquisition is running
            {
				// Mirroring
				if (m_pConfig->verticalMirroring)
				{
#ifndef NEXT_GEN_SYSTEM
					IppiSize roi_oct = { m_pViewTab->m_visImage.size(0), m_pViewTab->m_visImage.size(1) };
#else
					IppiSize roi_oct = { m_pConfig->octScansFFT / 2, m_pConfig->octAlines };
#endif
					ippiMirror_8u_C1IR(oct_data, roi_oct.width, roi_oct, ippAxsVertical);
				}

                // Draw A-lines
#ifndef NEXT_GEN_SYSTEM
				m_pViewTab->m_visImage = np::Uint8Array2(oct_data, m_pConfig->octScans, m_pConfig->octAlines);	
#else
				m_pViewTab->m_visImage = np::Uint8Array2(oct_data, m_pConfig->octScansFFT / 2, m_pConfig->octAlines);
#endif
				m_pViewTab->m_visIntensity = np::FloatArray2(flim_data + 0 * m_pConfig->flimAlines, m_pConfig->flimAlines, 4);
				m_pViewTab->m_visLifetime = np::FloatArray2(flim_data + 8 * m_pConfig->flimAlines, m_pConfig->flimAlines, 3);
				
                // Draw Images	
				emit m_pViewTab->drawImage(m_pViewTab->m_visImage.raw_ptr(),
					m_pViewTab->m_visIntensity.raw_ptr(), m_pViewTab->m_visLifetime.raw_ptr());
            }

			// Return (push) the buffer to the previous threading queue
            {
                std::unique_lock<std::mutex> lock(m_syncFlimVisualization.mtx);
                m_syncFlimVisualization.queue_buffer.push(flim_data);
            }
            {
                std::unique_lock<std::mutex> lock(m_syncOctVisualization.mtx);
                m_syncOctVisualization.queue_buffer.push(oct_data);
            }
        }
        else
        {
            if (flim_data != nullptr)
            {
                float* flim_temp = flim_data;
                do
                {
                    m_syncFlimVisualization.queue_buffer.push(flim_temp);
                    flim_temp = m_syncFlimVisualization.Queue_sync.pop();
                } while (flim_temp != nullptr);
            }
            else if (oct_data != nullptr)
            {
                uint8_t* oct_temp = oct_data;
                do
                {
                    m_syncOctVisualization.queue_buffer.push(oct_temp);
                    oct_temp = m_syncOctVisualization.Queue_sync.pop();
                } while (oct_temp != nullptr);
            }

            m_pThreadVisualization->_running = false;
        }

        (void)frame_count;
    };

    m_pThreadVisualization->DidStopData += [&]() {
        // None
    };

    m_pThreadVisualization->SendStatusMessage += [&](const char* msg, bool is_error) {

		QString qmsg = QString::fromUtf8(msg);
		if (is_error)
		{
			m_pConfig->writeToLog(QString("[ERROR] %1").arg(qmsg));
			QMessageBox MsgBox(QMessageBox::Critical, "Thread Mananger Error", qmsg);
			MsgBox.exec();

			if (m_pDataAcquisition->getAcquisitionState())
			{
				enableDeviceControl(false);
				enableDataAcquisition(false);
			}
		}
		else
		{
			m_pConfig->writeToLog(qmsg);
			qDebug() << qmsg;
		}
    };
}


void QStreamTab::createSettingDlg()
{
    if (m_pSettingDlg == nullptr)
    {
        m_pSettingDlg = new SettingDlg(this);
        connect(m_pSettingDlg, SIGNAL(finished(int)), this, SLOT(deleteSettingDlg()));
		m_pSettingDlg->show(); // modal-less
    }
	m_pSettingDlg->raise();
	m_pSettingDlg->activateWindow();
}

void QStreamTab::deleteSettingDlg()
{
	m_pSettingDlg->deleteLater();
    m_pSettingDlg = nullptr;
}


void QStreamTab::resetCatheterCalibration()
{
	std::thread calib([&]() {
		if (m_pDeviceControl->getAxsunControl())
		{
			m_pConfig->axsunVDLLength = 0.0;
			m_pScrollBar_CatheterCalibration->setValue(0);
#ifdef AXSUN_ENABLE
			m_pDeviceControl->getAxsunControl()->setVDLHome();
#endif
		}
	});
	calib.detach();	

	m_pScrollBar_InnerOffset->setValue(0);
}

void QStreamTab::scrollCatheterCalibration(int value)
{
	if (m_pDeviceControl->getAxsunControl())
	{
		m_pConfig->axsunVDLLength = (float)value / 100.0f;
#ifdef AXSUN_ENABLE
		m_pDeviceControl->getAxsunControl()->setVDLLength(m_pConfig->axsunVDLLength);
#endif
	}
}

void QStreamTab::scrollInnerOffsetLength(int offset)
{
	m_pConfig->innerOffsetLength = offset;
}


void QStreamTab::enableRotation(bool enabled)
{	
    if (enabled)
    {	
		// Set widgets
        m_pToggleButton_EnableRotation->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
        m_pToggleButton_EnableRotation->setText(" Disable Rotation");
        m_pToggleButton_EnableRotation->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#ff0000}");
		m_pToggleButton_StartPullback->setEnabled(true);

		if (m_pDeviceControl->getRotatyMotor())
		{			
			if (m_pConfig->axsunPipelineMode == 0)
			{
				m_pConfig->rotaryRpm = int(ROTATION_100FPS / REDUCED_COUNT);
				m_pDeviceControl->setSubSampling(REDUCED_COUNT);
			}
			else
				m_pConfig->rotaryRpm = int(ROTATION_100FPS / RAW_SUBSAMPLING);

			m_pDeviceControl->changeRotaryRpm(m_pConfig->rotaryRpm);
		}
		else
		{
			enableRotation(false);
			return;
		}

		m_pConfig->writeToLog(QString("Rotation started: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));
    }
    else
    {
		if (m_pDeviceControl->getRotatyMotor())
		{
			if (m_pConfig->axsunPipelineMode == 0)
				m_pDeviceControl->setSubSampling(1);
			m_pDeviceControl->rotateStop();
		}

        m_pToggleButton_EnableRotation->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
        m_pToggleButton_EnableRotation->setText(" Enable Rotation");
		m_pToggleButton_EnableRotation->setChecked(false);
        m_pToggleButton_EnableRotation->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#00ff00}");
		m_pToggleButton_StartPullback->setDisabled(true);

		m_pConfig->writeToLog(QString("Rotation stopped: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));
    }	
}

void QStreamTab::startPullback(bool enabled)
{
    if (enabled)
    {
		// Set widgets
        m_pToggleButton_StartPullback->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
        m_pToggleButton_StartPullback->setText(" Stop Pullback");
        m_pToggleButton_StartPullback->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#ff0000}");
		
		// Allow faster rotation
		if (m_pDeviceControl->getRotatyMotor())
		{			
			if (m_pConfig->axsunPipelineMode == 0)
			{
				m_pConfig->rotaryRpm = int(ROTATION_100FPS);
				m_pDeviceControl->setSubSampling(1);
			}
			else
				m_pConfig->rotaryRpm = int(ROTATION_100FPS / RAW_SUBSAMPLING);

			m_pDeviceControl->changeRotaryRpm(m_pConfig->rotaryRpm);
		}
		else
		{
			startPullback(false);
			return;
		}
		
		// Check pullback proceeding
		QMessageBox MsgBox(QMessageBox::Information, "OCT-FLIm", QString("Please push 'pullback' button to proceed...\n(pullback: %1 mm/s, %2 mm)")
			.arg(m_pConfig->pullbackSpeed, 2, 'f', 1).arg(m_pConfig->pullbackLength, 2, 'f', 1), QMessageBox::NoButton, 0);
		MsgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		MsgBox.setDefaultButton(QMessageBox::Ok);
		
		if (!m_pConfig->autoPullbackMode)
			MsgBox.setButtonText(QMessageBox::Ok, "Pullback");
		else
			MsgBox.setButtonText(QMessageBox::Ok, QString("Pullback (%1)").arg(m_pConfig->autoPullbackTime));
		MsgBox.move(446, 800);
		
		QTimer* pPullbackTimer = nullptr;
		if (m_pConfig->autoPullbackMode)
		{
			pPullbackTimer = new QTimer(this);
			pPullbackTimer->start(1000);
			int autoPullbackTime = m_pConfig->autoPullbackTime;
			connect(pPullbackTimer, &QTimer::timeout, [&]() {
				MsgBox.setButtonText(QMessageBox::Ok, QString("Pullback (%1)").arg(--autoPullbackTime));
				if (autoPullbackTime == 0)
					MsgBox.done(QMessageBox::Ok);
			});
		}
		int ret = MsgBox.exec();

		switch (ret)
		{
		case QMessageBox::Ok:

			// Kill timer first if needed
			if (m_pConfig->autoPullbackMode)
				if (pPullbackTimer)
					pPullbackTimer->stop();

			// Start recording (+ automatical pullback)
			if (!m_pDeviceControl->getPullbackMotor())
			{
				startPullback(false);
				return;
			}

			// Set recording - pullback synchronization	
			m_pMemoryBuffer->DidPullback.clear();
			m_pMemoryBuffer->DidPullback += [&]() {
				if (m_pDeviceControl->getPullbackMotor())
				{
					m_pDeviceControl->getPullbackMotor()->DidRotateEnd.clear();
					m_pDeviceControl->getPullbackMotor()->DidRotateEnd += [&](int)
					{
						emit pullbackFinished(false);
					};
					if (!m_pConfig->pullbackFlag)
					{
						m_pConfig->pullbackFlag = true;
						m_pConfig->setConfigFile("Havana3.ini");
						m_pDeviceControl->moveAbsolute();
					}
				}
			};

			m_pMemoryBuffer->startRecording();

			m_pConfig->writeToLog(QString("Pullback recording started: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));

			break;

		case QMessageBox::Cancel:
			if (m_pConfig->autoPullbackMode)
				if (pPullbackTimer)
					pPullbackTimer->stop();

			disconnect(m_pToggleButton_StartPullback, SIGNAL(toggled(bool)), 0, 0);	
			enableRotation(true);
			m_pToggleButton_StartPullback->setChecked(false);
			m_pToggleButton_StartPullback->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
			m_pToggleButton_StartPullback->setText(" Start Pullback");
			m_pToggleButton_StartPullback->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#00ff00}");
			connect(m_pToggleButton_StartPullback, SIGNAL(toggled(bool)), this, SLOT(startPullback(bool)));

			break;
		}
    }
    else 
    {		
		// Stop motor operation
		m_pMemoryBuffer->stopRecording();
		m_pDeviceControl->rotateStop();

		// Pullback stage homing
		m_pDeviceControl->getPullbackMotor()->DidRotateEnd.clear();
		m_pDeviceControl->getPullbackMotor()->DidRotateEnd += [&](int timeout) {
			if (timeout)
			{
				m_pConfig->pullbackFlag = false;
				m_pConfig->setConfigFile("Havana3.ini");
			}
		};
		if (m_pConfig->autoHomeMode) m_pDeviceControl->home();

		m_pConfig->writeToLog(QString("Pullback recording stopped: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));

		// Saving recorded data
		QDateTime datetime = QDateTime::currentDateTime();
		m_recordInfo.date = datetime.toString("yyyy-MM-dd hh:mm:ss");
		m_recordInfo.filename = m_pConfig->dbPath + "/record/" + m_recordInfo.patientId + datetime.toString("/yyyy-MM-dd_hh-mm-ss");
		m_pMemoryBuffer->startSaving(m_recordInfo);
		emit requestReview(m_recordInfo.recordId);
		
		// Set widgets			
		m_pToggleButton_EnableRotation->setChecked(false);
		m_pToggleButton_StartPullback->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
		m_pToggleButton_StartPullback->setText(" Start Pullback");
		m_pToggleButton_StartPullback->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#00ff00}");

		m_pConfig->writeToLog(QString("Recorded data saved: %1 (ID: %2): %3 : record id: %4")
			.arg(m_recordInfo.patientName).arg(m_recordInfo.patientId).arg(m_recordInfo.date).arg(m_recordInfo.recordId));
    }
}

#ifdef DEVELOPER_MODE
void QStreamTab::onTimerSyncMonitor()
{
	size_t fp_bfn = getFlimProcessingBufferQueueSize();
	size_t op_bfn = getOctProcessingBufferQueueSize();
	size_t fv_bfn = getFlimVisualizationBufferQueueSize();
	size_t ov_bfn = getOctVisualizationBufferQueueSize();
#ifndef NEXT_GEN_SYSTEM
#ifdef AXSUN_ENABLE
	double oct_fps = m_pDataAcquisition->getAxsunCapture()->frameRate;	
	uint32_t dropped_packets = m_pDataAcquisition->getAxsunCapture()->dropped_packets;
#else
	double oct_fps = 0.0;
	uint32_t dropped_packets = 0;
#endif
	double flim_fps = m_pDataAcquisition->getDigitizer()->frameRate;

	m_pLabel_StreamingSyncStatus->setText(QString("\n[Sync]\nFP#: %1\nOP#: %2\nFV#: %3\nOV#: %4\nOCT: %5 fps\nFLIM: %6 fps\ndrop ptks: %7")
		.arg(fp_bfn, 3).arg(op_bfn, 3).arg(fv_bfn, 3).arg(ov_bfn, 3).arg(oct_fps, 3, 'f', 2).arg(flim_fps, 3, 'f', 2).arg(dropped_packets));
#else
	double oct_fps = m_pDataAcquisition->getOctDigitizer()->frameRate;
	double flim_fps = m_pDataAcquisition->getFlimDigitizer()->frameRate;

	m_pLabel_StreamingSyncStatus->setText(QString("\n[Sync]\nFP#: %1\nOP#: %2\nFV#: %3\nOV#: %4\nOCT: %5 fps\nFLIM: %6 fps")
		.arg(fp_bfn, 3).arg(op_bfn, 3).arg(fv_bfn, 3).arg(ov_bfn, 3).arg(oct_fps, 3, 'f', 2).arg(flim_fps, 3, 'f', 2));
#endif
}

void QStreamTab::onTimerLaserMonitor()
{
	m_pDeviceControl->sendLaserCommand((char*)"?");
	m_pDeviceControl->requestOctStatus();
}
#endif
