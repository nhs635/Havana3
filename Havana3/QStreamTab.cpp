
#include "QStreamTab.h"

#include <QSqlQuery>

#include <Havana3/MainWindow.h>
#include <Havana3/Configuration.h>
#include <Havana3/HvnSqlDataBase.h>
#include <Havana3/QViewTab.h>
#include <Havana3/Dialog/SettingDlg.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/ThreadManager.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>

#include <MemoryBuffer/MemoryBuffer.h>
#include <DeviceControl/DeviceControl.h>

#include <iostream>
#include <thread>
#include <chrono>

#ifdef VERTICAL_MIRRORING
#include <ippcore.h>
#include <ippi.h>
#include <ipps.h>
#endif


QStreamTab::QStreamTab(QString patient_id, QWidget *parent) :
    QDialog(parent), m_patientId(patient_id), m_pSettingDlg(nullptr)
{
	// Set main window objects
    m_pMainWnd = dynamic_cast<MainWindow*>(parent);
    m_pConfig = m_pMainWnd->m_pConfiguration;
    m_pHvnSqlDataBase = m_pMainWnd->m_pHvnSqlDataBase;

    // Create widgets for live streaming view
    createLiveStreamingViewWidgets();

    // Create data acquisition object
    m_pDataAcquisition = new DataAcquisition(m_pConfig);
	
    // Create thread managers for data processing
	m_pThreadFlimProcess = new ThreadManager("FLIm image process");
	m_pThreadVisualization = new ThreadManager("Visualization process");

    // Create buffers for threading operation
    m_syncFlimProcessing.allocate_queue_buffer(m_pConfig->flimScans, m_pConfig->flimAlines, PROCESSING_BUFFER_SIZE);  // FLIm Processing
    m_syncFlimVisualization.allocate_queue_buffer(11, m_pConfig->flimAlines, PROCESSING_BUFFER_SIZE);  // FLIm Visualization
    m_syncOctVisualization.allocate_queue_buffer(m_pConfig->octScans, m_pConfig->octAlines, PROCESSING_BUFFER_SIZE);  // OCT Visualization
	
    // Set signal object
    setFlimAcquisitionCallback();
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

    // Initialize & start live streaming
    if (enableDataAcquisition(true))
        qDebug() << "data acq";
    if (enableMemoryBuffer(true))
        qDebug() << "mem buff";
    if (enableDeviceControl(true))
        qDebug() << "device control";

}

QStreamTab::~QStreamTab()
{
	if (m_pThreadVisualization) delete m_pThreadVisualization;
	if (m_pThreadFlimProcess) delete m_pThreadFlimProcess;
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
    {
        QString command = QString("SELECT * FROM patients WHERE patient_id=%1").arg(m_patientId);

        m_pHvnSqlDataBase->queryDatabase(command, [&](QSqlQuery& _sqlQuery) {
            while (_sqlQuery.next())
            {
                m_pLabel_PatientInformation->setText(QString("<b>%1</b>"
                                                             "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>ID:</b> %2").
                                                             arg(_sqlQuery.value(1).toString() + ", " + _sqlQuery.value(0).toString()).
                                                             arg(_sqlQuery.value(3).toString()));
                setWindowTitle(QString("Live Streaming: %1").arg(_sqlQuery.value(1).toString() + ", " + _sqlQuery.value(0).toString()));
            }
        });
    }
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
    m_pScrollBar_CatheterCalibration->setValue(0);

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

    m_pPushButton_Setting = new QPushButton(this);
    m_pPushButton_Setting->setText("  Setting");
    m_pPushButton_Setting->setIcon(style()->standardIcon(QStyle::SP_FileDialogInfoView));
    m_pPushButton_Setting->setFixedSize(100, 25);

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
    connect(m_pScrollBar_CatheterCalibration, SIGNAL(valueChanged(int)), this, SLOT(scrollCatheterCalibration(int)));
    connect(m_pToggleButton_EnableRotation, SIGNAL(toggled(bool)), this, SLOT(enableRotation(bool)));
    connect(m_pToggleButton_StartPullback, SIGNAL(toggled(bool)), this, SLOT(startPullback(bool)));
    connect(m_pPushButton_Setting, SIGNAL(clicked(bool)), this, SLOT(createSettingDlg()));
}


void QStreamTab::changeTab(bool status)
{
//	if (status) // result -> stream
//		m_pViewTab->setWidgetsValue();
//	else // stream -> result
//	{
//		m_pOperationTab->setWidgetsStatus();
//		m_pDeviceControlTab->setControlsStatus();
//	}

    //	if (m_pDeviceControlTab->getFlimCalibDlgDlg()) m_pDeviceControlTab->getFlimCalibDlgDlg()->close();
}

bool QStreamTab::enableDataAcquisition(bool enabled)
{
    if (enabled) // Enable data acquisition object
    {
        if (m_pDataAcquisition->InitializeAcquistion())
        {
            // Start thread process
            m_pThreadVisualization->startThreading();
            m_pThreadFlimProcess->startThreading();

            // Start data acquisition
            if (m_pDataAcquisition->StartAcquisition())
                return true;
        }
    }

    // Disable data acquisition object
    m_pDataAcquisition->StopAcquisition();
    m_pThreadFlimProcess->stopThreading();
    m_pThreadVisualization->stopThreading();

    return enabled ? false : true;
}

bool QStreamTab::enableMemoryBuffer(bool enabled)
{
    if (enabled)
    {
        std::thread allocate_writing_buffer([&]() {
            m_pMemoryBuffer->allocateWritingBuffer();
        });
        allocate_writing_buffer.detach();
    }
    else
    {
//        std::thread deallocate_writing_buffer([&]() {
//            m_pMemoryBuffer->de allocateWritingBuffer();
//        });
//        allocate_writing_buffer.detach();
    }

    return true;
}

bool QStreamTab::enableDeviceControl(bool enabled)
{
    // Set helical scanning control
    if (!m_pDeviceControl->connectPullbackMotor(enabled)) return false;
    if (!m_pDeviceControl->connectRotaryMotor(enabled)) return false;

    // Set FLIm system control
    if (enabled) std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (!m_pDeviceControl->applyPmtGainVoltage(enabled)) return false;
    if (!m_pDeviceControl->connectFlimLaser(enabled)) return false;

    // Set OCT system control
    if (enabled) std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (!m_pDeviceControl->connectAxsunControl(enabled)) return false;

    // Set master synchronization control
    if (enabled) std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (!m_pDeviceControl->startSynchronization(enabled)) return false;

    return true;
}


void QStreamTab::setFlimAcquisitionCallback()
{
//    DataAcquisition* pDataAcq = m_pOperationTab->getDataAcq();
//	MemoryBuffer* pMemBuff = m_pOperationTab->getMemBuff();
    m_pDataAcquisition->ConnectDaqAcquiredFlimData([&](int frame_count, const np::Array<uint16_t, 2>& frame) {

        // Data transfer
        if (!(frame_count % RENEWAL_COUNT))
        {
            // Data transfer for FLIm processing
            const uint16_t* frame_ptr = frame.raw_ptr();

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
					memcpy(pulse_ptr, frame.raw_ptr(), sizeof(uint16_t) * m_pConfig->flimFrameSize);

					// Push to the copy queue for copying transfered data in copy thread
                    m_pMemoryBuffer->m_syncFlimBuffering.Queue_sync.push(pulse_ptr);
				}
			}
			else
			{
				// Finish recording when the buffer is full
                m_pMemoryBuffer->m_bIsRecording = false;
//				m_pOperationTab->setRecordingButton(false);
			}
		}
    });

    m_pDataAcquisition->ConnectDaqStopFlimData([&]() {
        m_syncFlimProcessing.Queue_sync.push(nullptr);
    });

    m_pDataAcquisition->ConnectDaqSendStatusMessage([&](const char * msg, bool is_error) {
//		if (is_error) m_pOperationTab->setAcquisitionButton(false);
		QString qmsg = QString::fromUtf8(msg);
//		emit sendStatusMessage(qmsg, is_error);
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
//				if (m_pDeviceControlTab->getFlimCalibDlgDlg())
//					emit m_pDeviceControlTab->getFlimCalibDlgDlg()->plotRoiPulse(pFLIm, 0);
				
                // Push the buffers to sync Queues
                m_syncFlimVisualization.Queue_sync.push(flim_ptr);
                //m_syncVisualization.n_exec++;

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
//		if (is_error) m_pOperationTab->setAcquisitionButton(false);
        QString qmsg = QString::fromUtf8(msg);
//        emit sendStatusMessage(qmsg, is_error);
    };
}

void QStreamTab::setOctProcessingCallback()
{
//	DataAcquisition* pDataAcq = m_pOperationTab->getDataAcq();
//	MemoryBuffer* pMemBuff = m_pOperationTab->getMemBuff();
    m_pDataAcquisition->ConnectAxsunAcquiredOctData([&](uint32_t frame_count, const np::Uint8Array2& frame) {
				
        // Mirroring
#ifdef VERTICAL_MIRRORING
        IppiSize roi_oct = { frame.size(0), frame.size(1) };
        ippiMirror_8u_C1IR((Ipp8u*)frame.raw_ptr(), roi_oct.width, roi_oct, ippAxsVertical);
#endif

        // Data transfer
        if (!(frame_count % RENEWAL_COUNT))
        {
            // Data transfer for FLIm processing
            const uint8_t* frame_ptr = frame.raw_ptr();

            // Get buffer from threading queue
            uint8_t* oct_ptr = nullptr;
            {
                std::unique_lock<std::mutex> lock(m_syncOctVisualization.mtx);

                if (!m_syncOctVisualization.queue_buffer.empty())
                {
                    oct_ptr = m_syncOctVisualization.queue_buffer.front();
                    m_syncOctVisualization.queue_buffer.pop();
                }
            }

            if (oct_ptr != nullptr)
            {
                // Body
                memcpy(oct_ptr, frame_ptr, sizeof(uint8_t) * m_pConfig->octFrameSize);

                // Push the buffer to sync Queue
                m_syncOctVisualization.Queue_sync.push(oct_ptr);
                //m_syncOctVisualization.n_exec++;
            }
        }

        // Buffering (When recording)
        if (m_pMemoryBuffer->m_bIsRecording)
        {
            if (m_pMemoryBuffer->m_nRecordedFrames < WRITING_BUFFER_SIZE)
            {
                // Get buffer from writing queue
                uint8_t* oct_ptr = nullptr;
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
                    memcpy(oct_ptr, frame.raw_ptr(), sizeof(uint8_t) * m_pConfig->octFrameSize);

                    // Push to the copy queue for copying transfered data in copy thread
                    m_pMemoryBuffer->m_syncOctBuffering.Queue_sync.push(oct_ptr);
                }
            }
        }
    });

    m_pDataAcquisition->ConnectAxsunStopOctData([&]() {
        m_syncOctVisualization.Queue_sync.push(nullptr);
    });

    m_pDataAcquisition->ConnectAxsunSendStatusMessage([&](const char * msg, bool is_error) {
//		if (is_error) m_pOperationTab->setAcquisitionButton(false);
        QString qmsg = QString::fromUtf8(msg);
//        emit sendStatusMessage(qmsg, is_error);
    });
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
//            if (m_pOperationTab->getAcquisitionButton()->isChecked()) // Only valid if acquisition is running
            {
                // Draw A-lines
                m_pViewTab->m_visImage = np::Uint8Array2(oct_data, m_pConfig->octScans, m_pConfig->octAlines);
                m_pViewTab->m_visIntensity = np::FloatArray2(flim_data + 0 * m_pConfig->flimAlines, m_pConfig->flimAlines, 4);
                m_pViewTab->m_visLifetime = np::FloatArray2(flim_data + 8 * m_pConfig->flimAlines, m_pConfig->flimAlines, 3);
				
                // Draw Images
                //m_pViewTab->visualizeImage(m_pViewTab->m_visImage.raw_ptr(),
                //		m_pViewTab->m_visIntensity.raw_ptr(), m_pViewTab->m_visLifetime.raw_ptr());
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
//		if (is_error) m_pOperationTab->setAcquisitionButton(false);
        QString qmsg = QString::fromUtf8(msg);
//        emit sendStatusMessage(qmsg, is_error);
    };
}


void QStreamTab::createSettingDlg()
{
    if (m_pSettingDlg == nullptr)
    {
        m_pSettingDlg = new SettingDlg(true, this);
        connect(m_pSettingDlg, SIGNAL(finished(int)), this, SLOT(deleteSettingDlg()));
        m_pSettingDlg->show();
    }
    m_pSettingDlg->raise();
    m_pSettingDlg->activateWindow();
}

void QStreamTab::deleteSettingDlg()
{
    m_pSettingDlg->deleteLater();
    m_pSettingDlg = nullptr;
}

void QStreamTab::scrollCatheterCalibration(int value)
{

}

void QStreamTab::enableRotation(bool enabled)
{
    if (enabled)
    {
        m_pToggleButton_EnableRotation->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
        m_pToggleButton_EnableRotation->setText(" Disable Rotation");
        m_pToggleButton_EnableRotation->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#ff0000}");
    }
    else
    {
        m_pToggleButton_EnableRotation->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
        m_pToggleButton_EnableRotation->setText(" Enable Rotation");
        m_pToggleButton_EnableRotation->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#00ff00}");
    }
}

void QStreamTab::startPullback(bool enabled)
{
    if (enabled)
    {
        m_pToggleButton_StartPullback->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
        m_pToggleButton_StartPullback->setText(" Stop Pullback");
        m_pToggleButton_StartPullback->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#ff0000}");
    }
    else
    {
        m_pToggleButton_StartPullback->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
        m_pToggleButton_StartPullback->setText(" Start Pullback");
        m_pToggleButton_StartPullback->setStyleSheet("QPushButton{font-size:12pt; font-weight:bold; color:black; background-color:#00ff00}");
    }
}



//void QStreamTab::processMessage(QString qmsg, bool is_error)
//{
////	m_pListWidget_MsgWnd->addItem(qmsg);
////	m_pListWidget_MsgWnd->setCurrentRow(m_pListWidget_MsgWnd->count() - 1);

////	if (is_error)
////	{
////		QMessageBox MsgBox(QMessageBox::Critical, "Error", qmsg);
////		MsgBox.exec();
////	}
//}
