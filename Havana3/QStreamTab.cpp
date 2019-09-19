
#include "QStreamTab.h"

#include <Havana3/MainWindow.h>
#include <Havana3/QOperationTab.h>
#include <Havana3/QDeviceControlTab.h>
#include <Havana3/QVisualizationTab.h>

#include <Havana3/Dialog/FlimCalibDlg.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/ThreadManager.h>

#include <DataAcquisition/FLImProcess/FLImProcess.h>

#include <MemoryBuffer/MemoryBuffer.h>

#ifdef VERTICAL_MIRRORING
#include <ippcore.h>
#include <ippi.h>
#include <ipps.h>
#endif


QStreamTab::QStreamTab(QWidget *parent) :
    QDialog(parent)
{
	// Set main window objects
    m_pMainWnd = dynamic_cast<MainWindow*>(parent);
    m_pConfig = m_pMainWnd->m_pConfiguration;

	// Create message window
	m_pListWidget_MsgWnd = new QListWidget(this);
	m_pListWidget_MsgWnd->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	m_pListWidget_MsgWnd->setStyleSheet("font: 7pt");
	m_pConfig->msgHandle += [&](const char* msg) {
		QString qmsg = QString::fromUtf8(msg);
		emit sendStatusMessage(qmsg, false);
	};
	connect(this, SIGNAL(sendStatusMessage(QString, bool)), this, SLOT(processMessage(QString, bool)));

    // Create streaming objects
    m_pOperationTab = new QOperationTab(this);
    m_pDeviceControlTab = new QDeviceControlTab(this);
    m_pVisualizationTab = new QVisualizationTab(true, this);

    // Create group boxes for streaming objects
    m_pGroupBox_OperationTab = new QGroupBox();
    m_pGroupBox_OperationTab->setLayout(m_pOperationTab->getLayout());
    m_pGroupBox_OperationTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    m_pGroupBox_OperationTab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    m_pGroupBox_DeviceControlTab = new QGroupBox();
    m_pGroupBox_DeviceControlTab->setLayout(m_pDeviceControlTab->getLayout());
    m_pGroupBox_DeviceControlTab->setTitle("  Device Control  ");
    m_pGroupBox_DeviceControlTab->setStyleSheet("QGroupBox { padding-top: 20px; margin-top: -10px; } QGroupBox::title { subcontrol-origin: margin; left: 8px; top: 2px; }");
    m_pGroupBox_DeviceControlTab->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pGroupBox_DeviceControlTab->setFixedWidth(341);

    m_pGroupBox_VisualizationTab = new QGroupBox();
    m_pGroupBox_VisualizationTab->setLayout(m_pVisualizationTab->getLayout());
    m_pGroupBox_VisualizationTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    m_pGroupBox_VisualizationTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	
    // Create thread managers for data processing
	m_pThreadFlimProcess = new ThreadManager("FLIm image process");
	m_pThreadVisualization = new ThreadManager("Visualization process");

    // Create buffers for threading operation
	m_syncFlimProcessing.allocate_queue_buffer(m_pConfig->flimScans, m_pConfig->flimAlines, PROCESSING_BUFFER_SIZE); // FLIm Processing
	m_syncFlimVisualization.allocate_queue_buffer(11, m_pConfig->flimAlines, PROCESSING_BUFFER_SIZE); // FLIm Visualization
	m_syncOctVisualization.allocate_queue_buffer(m_pConfig->octScans, m_pConfig->octAlines, PROCESSING_BUFFER_SIZE); // OCT Visualization
	
    // Set signal object
    setFlimAcquisitionCallback();
    setFlimProcessingCallback();
	setOctProcessingCallback();
    setVisualizationCallback();

	
    // Create layout
    QGridLayout* pGridLayout = new QGridLayout;
    pGridLayout->setSpacing(2);
	
    // Set layout    
    pGridLayout->addWidget(m_pGroupBox_VisualizationTab, 0, 0, 4, 1);
    pGridLayout->addWidget(m_pGroupBox_OperationTab, 0, 1);
    pGridLayout->addWidget(m_pVisualizationTab->getVisualizationWidgetsBox(), 1, 1);
    pGridLayout->addWidget(m_pGroupBox_DeviceControlTab, 2, 1);
	pGridLayout->addWidget(m_pListWidget_MsgWnd, 3, 1);

    this->setLayout(pGridLayout);
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

void QStreamTab::changeTab(bool status)
{
	if (status) // result -> stream
		m_pVisualizationTab->setWidgetsValue();
	else // stream -> result
	{
		m_pOperationTab->setWidgetsStatus();
		m_pDeviceControlTab->setControlsStatus();
	}

	if (m_pDeviceControlTab->getFlimCalibDlg()) m_pDeviceControlTab->getFlimCalibDlg()->close();
}

void QStreamTab::setFlimAcquisitionCallback()
{
    DataAcquisition* pDataAcq = m_pOperationTab->getDataAcq();
	MemoryBuffer* pMemBuff = m_pOperationTab->getMemBuff();
    pDataAcq->ConnectDaqAcquiredFlimData([&, pMemBuff](int frame_count, const np::Array<uint16_t, 2>& frame) {

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
		if (pMemBuff->m_bIsRecording)
		{
			if (pMemBuff->m_nRecordedFrames < WRITING_BUFFER_SIZE)
			{
				// Get buffer from writing queue
				uint16_t* pulse_ptr = nullptr;
				{
					std::unique_lock<std::mutex> lock(pMemBuff->m_syncFlimBuffering.mtx);

					if (!pMemBuff->m_syncFlimBuffering.queue_buffer.empty())
					{
						pulse_ptr = pMemBuff->m_syncFlimBuffering.queue_buffer.front();
						pMemBuff->m_syncFlimBuffering.queue_buffer.pop();
					}
				}

				if (pulse_ptr != nullptr)
				{
					// Body (Copying the frame data)
					memcpy(pulse_ptr, frame.raw_ptr(), sizeof(uint16_t) * m_pConfig->flimFrameSize);

					// Push to the copy queue for copying transfered data in copy thread
					pMemBuff->m_syncFlimBuffering.Queue_sync.push(pulse_ptr);
				}
			}
			else
			{
				// Finish recording when the buffer is full
				pMemBuff->m_bIsRecording = false;
				m_pOperationTab->setRecordingButton(false);
			}
		}
    });

    pDataAcq->ConnectDaqStopFlimData([&]() {
        m_syncFlimProcessing.Queue_sync.push(nullptr);
    });

    pDataAcq->ConnectDaqSendStatusMessage([&](const char * msg, bool is_error) {
		if (is_error) m_pOperationTab->setAcquisitionButton(false);
		QString qmsg = QString::fromUtf8(msg);
		emit sendStatusMessage(qmsg, is_error);
    });
}

void QStreamTab::setFlimProcessingCallback()
{
	// FLIm Process Signal Objects /////////////////////////////////////////////////////////////////////////////////////////
	FLImProcess *pFLIm = m_pOperationTab->getDataAcq()->getFLIm();
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
					// 일단은 circ shifting 형식으로 작업하지만, 최초 frame에서 밀어서 전체적으로 다 밀리는 형식으로 되야할 것임.
				}

				// Transfer to FLIm calibration dlg
				if (m_pDeviceControlTab->getFlimCalibDlg())
					emit m_pDeviceControlTab->getFlimCalibDlg()->plotRoiPulse(pFLIm, 0);
				
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
		if (is_error) m_pOperationTab->setAcquisitionButton(false);
		QString qmsg = QString::fromUtf8(msg);
		emit sendStatusMessage(qmsg, is_error);
	};
}

void QStreamTab::setOctProcessingCallback()
{
	DataAcquisition* pDataAcq = m_pOperationTab->getDataAcq();
	MemoryBuffer* pMemBuff = m_pOperationTab->getMemBuff();
	pDataAcq->ConnectAxsunAcquiredOctData([&, pMemBuff](uint32_t frame_count, const np::Uint8Array2& frame) {
				
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
		if (pMemBuff->m_bIsRecording)
		{
			if (pMemBuff->m_nRecordedFrames < WRITING_BUFFER_SIZE)
			{
				// Get buffer from writing queue
				uint8_t* oct_ptr = nullptr;
				{
					std::unique_lock<std::mutex> lock(pMemBuff->m_syncOctBuffering.mtx);

					if (!pMemBuff->m_syncOctBuffering.queue_buffer.empty())
					{
						oct_ptr = pMemBuff->m_syncOctBuffering.queue_buffer.front();
						pMemBuff->m_syncOctBuffering.queue_buffer.pop();
					}
				}

				if (oct_ptr != nullptr)
				{
					// Body (Copying the frame data)
					memcpy(oct_ptr, frame.raw_ptr(), sizeof(uint8_t) * m_pConfig->octFrameSize);

					// Push to the copy queue for copying transfered data in copy thread
					pMemBuff->m_syncOctBuffering.Queue_sync.push(oct_ptr);
				}
			}
		}
	});

	pDataAcq->ConnectAxsunStopOctData([&]() {
		m_syncOctVisualization.Queue_sync.push(nullptr);
	});

	pDataAcq->ConnectAxsunSendStatusMessage([&](const char * msg, bool is_error) {
		if (is_error) m_pOperationTab->setAcquisitionButton(false);
		QString qmsg = QString::fromUtf8(msg);
		emit sendStatusMessage(qmsg, is_error);
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
            if (m_pOperationTab->getAcquisitionButton()->isChecked()) // Only valid if acquisition is running
            {
				// Draw A-lines
				m_pVisualizationTab->m_visImage = np::Uint8Array2(oct_data, m_pConfig->octScans, m_pConfig->octAlines);
				m_pVisualizationTab->m_visIntensity = np::FloatArray2(flim_data + 0 * m_pConfig->flimAlines, m_pConfig->flimAlines, 4);
				m_pVisualizationTab->m_visLifetime = np::FloatArray2(flim_data + 8 * m_pConfig->flimAlines, m_pConfig->flimAlines, 3);
				
                // Draw Images
				emit m_pVisualizationTab->drawImage(m_pVisualizationTab->m_visImage.raw_ptr(),
					m_pVisualizationTab->m_visIntensity.raw_ptr(), m_pVisualizationTab->m_visLifetime.raw_ptr());
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
		if (is_error) m_pOperationTab->setAcquisitionButton(false);
        QString qmsg = QString::fromUtf8(msg);
        emit sendStatusMessage(qmsg, is_error);
    };
}



void QStreamTab::processMessage(QString qmsg, bool is_error)
{
	m_pListWidget_MsgWnd->addItem(qmsg);
	m_pListWidget_MsgWnd->setCurrentRow(m_pListWidget_MsgWnd->count() - 1);

	if (is_error)
	{
		QMessageBox MsgBox(QMessageBox::Critical, "Error", qmsg);
		MsgBox.exec();
	}
}
