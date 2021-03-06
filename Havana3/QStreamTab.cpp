
#include "QStreamTab.h"

#include <QSqlQuery>

#include <Havana3/MainWindow.h>
#include <Havana3/HvnSqlDataBase.h>
#include <Havana3/QViewTab.h>
#include <Havana3/Dialog/SettingDlg.h>
#include <Havana3/Dialog/FlimCalibTab.h>
#ifdef DEVELOPER_MODE
#include <Havana3/Viewer/QScope.h>
#endif

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/ThreadManager.h>
#ifndef NEXT_GEN_SYSTEM
#include <DataAcquisition/AxsunCapture/AxsunCapture.h>
#include <DataAcquisition/SignatecDAQ/SignatecDAQ.h>
#else
#include <DataAcquisition/AlazarDAQ/AlazarDAQ.h>
#endif
#include <DataAcquisition/FLImProcess/FLImProcess.h>

#include <MemoryBuffer/MemoryBuffer.h>

#include <DeviceControl/DeviceControl.h>
#include <DeviceControl/FaulhaberMotor/RotaryMotor.h>
#include <DeviceControl/FaulhaberMotor/PullbackMotor.h>
#include <DeviceControl/AxsunControl/AxsunControl.h>

#include <iostream>
#include <thread>
#include <chrono>

#include <ippcore.h>
#include <ippi.h>
#include <ipps.h>


QStreamTab::QStreamTab(QString patient_id, QWidget *parent) :
    QDialog(parent), m_bFirstImplemented(true), m_pSettingDlg(nullptr), m_pScope_Alines(nullptr)
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
	m_syncOctProcessing.allocate_queue_buffer(m_pConfig->octScans, m_pConfig->octAlines, PROCESSING_BUFFER_SIZE); // OCT Processing
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
	///connect(m_pMemoryBuffer, &MemoryBuffer::finishedBufferAllocation, [&]() { m_pToggleButton_StartPullback->setEnabled(true); });
	///(m_pMemoryBuffer, &MemoryBuffer::finishedWritingThread, [&]() { emit requestReview(m_recordInfo.recordId); });

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
	if (m_pDeviceControl) delete m_pDeviceControl;
	if (m_pDataAcquisition) delete m_pDataAcquisition;

	if (m_pThreadVisualization) delete m_pThreadVisualization;
	if (m_pThreadOctProcess) delete m_pThreadOctProcess;
	if (m_pThreadFlimProcess) delete m_pThreadFlimProcess;
	
	m_syncFlimProcessing.deallocate_queue_buffer();
	m_syncOctProcessing.deallocate_queue_buffer();
	m_syncFlimVisualization.deallocate_queue_buffer();
	m_syncOctVisualization.deallocate_queue_buffer();

#ifdef DEVELOPER_MODE
	if (m_pScope_Alines)
	{
		m_pScope_Alines->close();
		m_pScope_Alines->deleteLater();
	}
	m_pSyncMonitorTimer->stop();
#endif
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

    m_pLabel_CatheterCalibration = new QLabel(this);
    m_pLabel_CatheterCalibration->setText("Catheter\nCalibration");
    m_pLabel_CatheterCalibration->setAlignment(Qt::AlignCenter);

    m_pScrollBar_CatheterCalibration = new QScrollBar(this);
    m_pScrollBar_CatheterCalibration->setFixedSize(200, 32);
    m_pScrollBar_CatheterCalibration->setOrientation(Qt::Horizontal);
    m_pScrollBar_CatheterCalibration->setRange(0, 1500);
    m_pScrollBar_CatheterCalibration->setSingleStep(5);
    m_pScrollBar_CatheterCalibration->setPageStep(50);
    m_pScrollBar_CatheterCalibration->setValue((int)(m_pConfig->axsunVDLLength * 100.0f));

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
	//m_pToggleButton_StartPullback->setDisabled(true);

    m_pPushButton_Setting = new QPushButton(this);
    m_pPushButton_Setting->setText("  Setting");
    m_pPushButton_Setting->setIcon(style()->standardIcon(QStyle::SP_FileDialogInfoView));
    m_pPushButton_Setting->setFixedSize(100, 25);

#ifdef DEVELOPER_MODE
#ifndef NEXT_GEN_SYSTEM
	m_pScope_Alines = new QScope({ 0, (double)m_pConfig->octScans }, { 0.0, 255.0 }, 2, 2, 1, 1, 0, 0, "", "", false, false, this);
#else
	m_pScope_Alines = new QScope({ 0, (double)m_pConfig->octScansFFT / 2.0 }, { m_pConfig->axsunDbRange.min, m_pConfig->axsunDbRange.max }, 2, 2, 1, 1, 0, 0, "", "dB", false, false, this);
#endif
	m_pScope_Alines->setWindowTitle("OCT A-lines");
	m_pScope_Alines->setFixedSize(640, 240);
	connect(this, SIGNAL(plotAline(const float*)), m_pScope_Alines, SLOT(drawData(const float*)));
	m_pScope_Alines->show();

	m_pLabel_StreamingSyncStatus = new QLabel(this);
	m_pLabel_StreamingSyncStatus->setFixedSize(150, 200);
	m_pLabel_StreamingSyncStatus->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	
	m_pSyncMonitorTimer = new QTimer(this);
	m_pSyncMonitorTimer->start(1000);
	connect(m_pSyncMonitorTimer, SIGNAL(timeout()), this, SLOT(onTimerSyncMonitor()));
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

    QHBoxLayout *pHBoxLayout_Operation = new QHBoxLayout;
    pHBoxLayout_Operation->setSpacing(5);
    pHBoxLayout_Operation->setAlignment(Qt::AlignVCenter);
    pHBoxLayout_Operation->addWidget(m_pLabel_CatheterCalibration);
    pHBoxLayout_Operation->addWidget(m_pScrollBar_CatheterCalibration);
    pHBoxLayout_Operation->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_Operation->addWidget(m_pToggleButton_EnableRotation);
    pHBoxLayout_Operation->addWidget(m_pToggleButton_StartPullback);

    pVBoxLayout_LiveStreaming->addItem(pHBoxLayout_Title);
    pVBoxLayout_LiveStreaming->addWidget(m_pViewTab->getViewWidget());
    pVBoxLayout_LiveStreaming->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
    pVBoxLayout_LiveStreaming->addItem(pHBoxLayout_Operation);

    m_pGroupBox_LiveStreaming->setLayout(pVBoxLayout_LiveStreaming);


    // Connect signal and slot
	connect(this, SIGNAL(getCapture(QByteArray &)), m_pViewTab, SLOT(getCapture(QByteArray &)));
    connect(m_pScrollBar_CatheterCalibration, SIGNAL(valueChanged(int)), this, SLOT(scrollCatheterCalibration(int)));
    connect(m_pToggleButton_EnableRotation, SIGNAL(toggled(bool)), this, SLOT(enableRotation(bool)));
    connect(m_pToggleButton_StartPullback, SIGNAL(toggled(bool)), this, SLOT(startPullback(bool)));
    connect(m_pPushButton_Setting, SIGNAL(clicked(bool)), this, SLOT(createSettingDlg()));
}


void QStreamTab::changePatient(QString patient_id)
{
	m_recordInfo.patientId = patient_id;

	QString command = QString("SELECT * FROM patients WHERE patient_id=%1").arg(m_recordInfo.patientId);

	m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {
		while (_sqlQuery.next())
		{
			m_recordInfo.patientName = _sqlQuery.value(1).toString() + ", " + _sqlQuery.value(0).toString();
			m_pLabel_PatientInformation->setText(QString("<b>%1</b>"
				"&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>ID:</b> %2").
				arg(m_recordInfo.patientName).
				arg(m_recordInfo.patientId));
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
		msg_box.move((m_pMainWnd->width() - msg_box.width()) / 2, (m_pMainWnd->height() - msg_box.height()) / 2);
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
		
		m_pConfig->writeToLog(QString("Live streaming started: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));
	}
	else
	{
		enableMemoryBuffer(false);
		enableDeviceControl(false);
		enableDataAcquisition(false);

		m_pConfig->writeToLog(QString("Live streaming stopped: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));
	}
}


bool QStreamTab::enableMemoryBuffer(bool enabled)
{
	if (enabled)
	{
        std::thread allocate_buffers([&]() {
            // Create buffers for data recording
            m_pMemoryBuffer->allocateWritingBuffer();
		});
        allocate_buffers.detach();
	}
	else
    {
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
//		if (!m_pDeviceControl->connectRotaryMotor(true)) return false;
//		if (!m_pDeviceControl->connectPullbackMotor(true)) return false;

		// Set FLIm system control
		if (!m_pDeviceControl->connectFlimLaser(true)) return false;
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
			m_pDeviceControl->setLiveImaging(true);
#endif
			m_pDeviceControl->setLightSource(true);
		}
	
#ifdef DEVELOPER_MODE
	m_pScope_Alines->raise();
	m_pScope_Alines->activateWindow();
#endif
	}
	else
	{
		// Turn off the master trigger generation
		m_pDeviceControl->setLightSource(false);
#ifndef NEXT_GEN_SYSTEM
		m_pDeviceControl->setLiveImaging(false);
#endif

		// Turn off all the devices
		m_pDeviceControl->setAllDeviceOff();
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
        if (!(frame_count % RENEWAL_COUNT))
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

        // Mirroring
		if (m_pConfig->verticalMirroring)
		{
#ifndef NEXT_GEN_SYSTEM
			IppiSize roi_oct = { frame.size(0), frame.size(1) };
			ippiMirror_8u_C1IR((Ipp8u*)frame.raw_ptr(), roi_oct.width, roi_oct, ippAxsVertical);
#else
			IppiSize roi_oct = { m_pConfig->octScansFFT / 2, m_pConfig->octAlines };
			ippiMirror_32f_C1IR((Ipp32f*)_frame_ptr, roi_oct.width, roi_oct, ippAxsVertical);
#endif
		}

		// Data transfer
		if (!(frame_count % RENEWAL_COUNT))
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
				memcpy(oct_ptr, frame_ptr, sizeof(uint8_t) * m_pConfig->octFrameSize);
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
#ifndef NEXT_GEN_SYSTEM
					memcpy(oct_ptr, frame.raw_ptr(), sizeof(uint8_t) * m_pConfig->octFrameSize);
#else
					memcpy(oct_ptr, (float*)_frame_ptr, sizeof(float) * m_pConfig->octFrameSize);
#endif

					// Push to the copy queue for copying transfered data in copy thread
					m_pMemoryBuffer->m_syncOctBuffering.Queue_sync.push(oct_ptr);
				}
			}
		}
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

                for (int i = 0; i < 11; i++)
                {
                    float* pValue = flim_ptr + i * m_pConfig->flimAlines;
                    std::rotate(pValue, pValue + INTRA_FRAME_SYNC, pValue + m_pConfig->flimAlines);
                }

                // Transfer to FLIm calibration dlg
				if (m_pSettingDlg)
				{
					if (m_pSettingDlg->getTabWidget()->currentIndex() == 2) // FLIm calibration view
						emit m_pSettingDlg->getFlimCalibTab()->plotRoiPulse(pFLIm, 0);
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
	m_pThreadOctProcess->DidAcquireData += [&](int frame_count) {

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
				memcpy(img_ptr, oct_data, sizeof(uint8_t) * m_pConfig->octFrameSize);
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
					np::FloatArray aline(m_pConfig->octScans);
					ippsConvert_8u32f(img_ptr, aline, m_pConfig->octScans);
					emit plotAline(aline);
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
    m_pThreadVisualization->DidAcquireData += [&](int frame_count) {
		
        // Get the buffers from the previous sync Queues
        float* flim_data = m_syncFlimVisualization.Queue_sync.pop();
        uint8_t* oct_data = m_syncOctVisualization.Queue_sync.pop();
        if ((flim_data != nullptr) && (oct_data != nullptr)) 
        {
            // Body
            if (m_pDataAcquisition->getAcquisitionState()) // Only valid if acquisition is running
            {
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
		m_pSettingDlg->setModal(true);
		m_pSettingDlg->exec();
    }
}

void QStreamTab::deleteSettingDlg()
{
	m_pSettingDlg->deleteLater();
    m_pSettingDlg = nullptr;
}


void QStreamTab::scrollCatheterCalibration(int value)
{
	/* 추후 VDL 작동 확인 하기.  더 알맞은 functionality implementation이 있는지 찾아 보기 */
	std::thread calib([&]() {
		if (m_pDeviceControl->getAxsunControl())
		{
			m_pConfig->axsunVDLLength = (float)value / 100.0f;
			m_pDeviceControl->getAxsunControl()->setVDLLength(m_pConfig->axsunVDLLength);
		}
	});
	calib.detach();
}

void QStreamTab::enableRotation(bool enabled)
{	
    if (enabled)
    {	
		// Set widgets
        m_pToggleButton_EnableRotation->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
        m_pToggleButton_EnableRotation->setText(" Disable Rotation");
        m_pToggleButton_EnableRotation->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#ff0000}");

		if (!m_pDeviceControl->connectRotaryMotor(true))
		{
			enableRotation(false);
			return;
		}

		m_pConfig->writeToLog(QString("Rotation started: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));
    }
    else
    {
		m_pDeviceControl->rotate(enabled);
		m_pDeviceControl->connectRotaryMotor(false);

        m_pToggleButton_EnableRotation->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
        m_pToggleButton_EnableRotation->setText(" Enable Rotation");
        m_pToggleButton_EnableRotation->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#00ff00}");

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
		
		//// Start recording (+ automatical pullback)
		//if (!m_pDeviceControl->connectPullbackMotor(true))
		//{
		//	startPullback(false);
		//	return;
		//}

		//// Set recording - pullback synchronization			
		//m_pMemoryBuffer->DidPullback += [&]() {
		//	if (m_pDeviceControl->getPullbackMotor())
		//	{
		//		m_pDeviceControl->getPullbackMotor()->DidRotateEnd += [&]() { startPullback(false);	};
		//		m_pDeviceControl->pullback();
		//	}
		//};
		m_pMemoryBuffer->startRecording();
		
		m_pConfig->writeToLog(QString("Pullback recording started: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));
    }
    else 
    {		
		// Stop motor operation
		m_pMemoryBuffer->stopRecording();
		m_pDeviceControl->rotate(false);
		//startLiveImaging(false);

		m_pConfig->writeToLog(QString("Pullback recording stopped: %1 (ID: %2)").arg(m_recordInfo.patientName).arg(m_recordInfo.patientId));

		// Saving recorded data
		QDateTime datetime = QDateTime::currentDateTime();
		m_recordInfo.date = datetime.toString("yyyy-MM-dd hh:mm:ss");
		m_recordInfo.filename = m_pConfig->dbPath + "/record/" + m_recordInfo.patientId + datetime.toString("/yyyy-MM-dd_hh-mm-ss");
		m_pMemoryBuffer->startSaving(m_recordInfo);
		emit requestReview(m_recordInfo.recordId);
		
		// Set widgets
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
	double oct_fps = m_pDataAcquisition->getAxsunCapture()->frameRate;
	double flim_fps = m_pDataAcquisition->getDigitizer()->frameRate;
	uint32_t dropped_packets = m_pDataAcquisition->getAxsunCapture()->dropped_packets;

	m_pLabel_StreamingSyncStatus->setText(QString("[Sync]\nFP#: %1\nOP#: %2\nFV#: %3\nOV#: %4\nOCT: %5 fps\nFLIM: %6 fps\ndrop ptks: %7")
		.arg(fp_bfn, 3).arg(op_bfn, 3).arg(fv_bfn, 3).arg(ov_bfn, 3).arg(oct_fps, 3, 'f', 2).arg(flim_fps, 3, 'f', 2).arg(dropped_packets));
#else
	double oct_fps = m_pDataAcquisition->getOctDigitizer()->frameRate;
	double flim_fps = m_pDataAcquisition->getFlimDigitizer()->frameRate;

	m_pLabel_StreamingSyncStatus->setText(QString("[Sync]\nFP#: %1\nOP#: %2\nFV#: %3\nOV#: %4\nOCT: %5 fps\nFLIM: %6 fps")
		.arg(fp_bfn, 3).arg(op_bfn, 3).arg(fv_bfn, 3).arg(ov_bfn, 3).arg(oct_fps, 3, 'f', 2).arg(flim_fps, 3, 'f', 2));
#endif
}
#endif
