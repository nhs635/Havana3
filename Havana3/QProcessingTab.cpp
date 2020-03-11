
#include "QProcessingTab.h"

#include <Havana3/Configuration.h>
#include <Havana3/MainWindow.h>
#include <Havana3/QResultTab.h>
#include <Havana3/QVisualizationTab.h>

#include <Havana3/Dialog/SaveResultDlg.h>
#include <Havana3/Dialog/PulseReviewDlg.h>
#include <Havana3/Dialog/LongitudinalViewDlg.h>

#include <Havana3/Viewer/QImageView.h>

#include <DataAcquisition/FLImProcess/FLImProcess.h>

#include <iostream>
#include <thread>


QProcessingTab::QProcessingTab(QWidget *parent) :
    QDialog(parent),
	m_pConfigTemp(nullptr), m_pFLIm(nullptr), m_pSaveResultDlg(nullptr) 
{
	// Set main window objects
    m_pResultTab = dynamic_cast<QResultTab*>(parent);
    m_pConfig = m_pResultTab->getMainWnd()->m_pConfiguration;

    // Create widgets for data processing
	m_pPushButton_StartProcessing = new QPushButton(this);
	m_pPushButton_StartProcessing->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	m_pPushButton_StartProcessing->setFixedHeight(30);
	m_pPushButton_StartProcessing->setText("Start Processing");

	m_pPushButton_SaveResults = new QPushButton(this);
	m_pPushButton_SaveResults->setFixedHeight(30);
	m_pPushButton_SaveResults->setText("Save Results...");
	m_pPushButton_SaveResults->setDisabled(true);
		
	// Create progress bar
	m_pProgressBar_PostProcessing = new QProgressBar(this);
	m_pProgressBar_PostProcessing->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	m_pProgressBar_PostProcessing->setFormat("");
	m_pProgressBar_PostProcessing->setValue(0);
	
    // Set layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(2);
	
    QHBoxLayout *pHBoxLayout_Processing = new QHBoxLayout;
	pHBoxLayout_Processing->setSpacing(2);

	pHBoxLayout_Processing->addWidget(m_pPushButton_StartProcessing);
	pHBoxLayout_Processing->addWidget(m_pPushButton_SaveResults);
	
    m_pVBoxLayout->addItem(pHBoxLayout_Processing);
    m_pVBoxLayout->addWidget(m_pProgressBar_PostProcessing);
    m_pVBoxLayout->addStretch(1);

    setLayout(m_pVBoxLayout);


	// Connect signal and slot
	connect(m_pPushButton_StartProcessing, SIGNAL(clicked(bool)), this, SLOT(startProcessing(void)));
	connect(m_pPushButton_SaveResults, SIGNAL(clicked(bool)), this, SLOT(createSaveResultDlg()));
	
	connect(this, SIGNAL(processedSingleFrame(int)), m_pProgressBar_PostProcessing, SLOT(setValue(int)));
	connect(this, SIGNAL(setWidgets(bool, Configuration*)), this, SLOT(setWidgetsEnabled(bool, Configuration*)));
}

QProcessingTab::~QProcessingTab()
{
	if (m_pSaveResultDlg) m_pSaveResultDlg->close();
}


void QProcessingTab::setWidgetsStatus()
{	
	if (m_pSaveResultDlg) m_pSaveResultDlg->close();
}


void QProcessingTab::createSaveResultDlg()
{
	if (m_pSaveResultDlg == nullptr)
	{
		m_pSaveResultDlg = new SaveResultDlg(this);
		connect(m_pSaveResultDlg, SIGNAL(finished(int)), this, SLOT(deleteSaveResultDlg()));
		m_pSaveResultDlg->show();
	}
	m_pSaveResultDlg->raise();
	m_pSaveResultDlg->activateWindow();
}

void QProcessingTab::deleteSaveResultDlg()
{
	m_pSaveResultDlg->deleteLater();
	m_pSaveResultDlg = nullptr;
}

void QProcessingTab::setWidgetsEnabled(bool enabled, Configuration* pConfig)
{
	if (!pConfig)
		m_pResultTab->getMainWnd()->m_pTabWidget->setEnabled(enabled);

	m_pPushButton_StartProcessing->setEnabled(enabled);
	m_pPushButton_SaveResults->setEnabled(enabled);
	
	if (!enabled)
	{
		if (pConfig)
		{
			m_pProgressBar_PostProcessing->setFormat("External data processing... %p%");
			m_pProgressBar_PostProcessing->setRange(0, pConfig->frames - 1);
		}
		else
		{
			m_pProgressBar_PostProcessing->setFormat("Save result images... %p%");
			m_pProgressBar_PostProcessing->setRange(0, (int)m_pResultTab->getVisualizationTab()->m_vectorOctImage.size() - 1);
		}

	}
	else
		m_pProgressBar_PostProcessing->setFormat("");
	m_pProgressBar_PostProcessing->setValue(0);
}


void QProcessingTab::startProcessing()
{
	/// m_pSlider_SelectFrame->setValue(0);

	if (m_pSaveResultDlg) m_pSaveResultDlg->close();
	if (getResultTab()->getVisualizationTab()->getPulseReviewDlg()) getResultTab()->getVisualizationTab()->getPulseReviewDlg()->close();
	if (getResultTab()->getVisualizationTab()->getLongitudinalViewDlg()) getResultTab()->getVisualizationTab()->getLongitudinalViewDlg()->close();

	// Get path to read
	QString fileName = QFileDialog::getOpenFileName(nullptr, "Load external FLIm OCT data", "", "FLIm OCT raw data (*.data)");
	if (fileName != "")
	{
		std::thread t1([&, fileName]() {
			
			///std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

			QFile file(fileName);
			if (false == file.open(QFile::ReadOnly))
				printf("[ERROR] Invalid external data!\n");
			else
			{
				// Read Ini File & Initialization ///////////////////////////////////////////////////////////
				QString fileTitle;
				for (int i = 0; i < fileName.length(); i++)
					if (fileName.at(i) == QChar('.')) fileTitle = fileName.left(i);
				for (int i = 0; i < fileName.length(); i++)
					if (fileName.at(i) == QChar('/')) m_path = fileName.left(i);

				QString iniName = fileTitle + ".ini";
				QString maskName = fileTitle + ".flim_mask";
				
				if (m_pConfigTemp) delete m_pConfigTemp;
				m_pConfigTemp = new Configuration;

				m_pConfigTemp->getConfigFile(iniName);
				m_pConfigTemp->frames = (int)(file.size() / (sizeof(uint8_t) * (qint64)m_pConfigTemp->octFrameSize + sizeof(uint16_t) * (qint64)m_pConfigTemp->flimFrameSize));
				m_pConfigTemp->frames -= m_pConfigTemp->interFrameSync;

				/// printf("Start external image processing... (Total nFrame: %d)\n", m_pConfigTemp->frames);

				// Set Widgets //////////////////////////////////////////////////////////////////////////////
				emit setWidgets(false, m_pConfigTemp);
				emit m_pResultTab->getVisualizationTab()->setWidgets(false, m_pConfigTemp);

				// Set Buffers //////////////////////////////////////////////////////////////////////////////
				m_pResultTab->getVisualizationTab()->setBuffers(m_pConfigTemp);
				
				m_syncDeinterleaving.allocate_queue_buffer(m_pConfigTemp->flimScans + m_pConfigTemp->octScans, m_pConfigTemp->octAlines, PROCESSING_BUFFER_SIZE);
				m_syncFlimProcessing.allocate_queue_buffer(m_pConfigTemp->flimScans, m_pConfigTemp->flimAlines, PROCESSING_BUFFER_SIZE);

				// Set FLIm Object ///////////////////////////////////////////////////////////////////////////
				if (m_pFLIm) delete m_pFLIm;
				m_pFLIm = new FLImProcess;
				m_pFLIm->setParameters(m_pConfigTemp);
				m_pFLIm->_resize(np::Uint16Array2(m_pConfigTemp->flimScans, m_pConfigTemp->flimAlines), m_pFLIm->_params);
				m_pFLIm->loadMaskData(maskName);

				// Get external data ////////////////////////////////////////////////////////////////////////
				std::thread load_data([&]() { loadingRawData(&file, m_pConfigTemp); });

				// Data DeInterleaving //////////////////////////////////////////////////////////////////////
				std::thread deinterleave([&]() { deinterleaving(m_pConfigTemp); });

				// FLIm Process /////////////////////////////////////////////////////////////////////////////
				std::thread flim_proc([&]() { flimProcessing(m_pFLIm, m_pConfigTemp); });

				// Wait for threads end /////////////////////////////////////////////////////////////////////
				load_data.join();
				deinterleave.join();
				flim_proc.join();

				// Generate en face maps ////////////////////////////////////////////////////////////////////
				getOctProjection(m_pResultTab->getVisualizationTab()->m_vectorOctImage, m_pResultTab->getVisualizationTab()->m_octProjection);

				// Delete OCT FLIM Object & threading sync buffers //////////////////////////////////////////
				m_syncDeinterleaving.deallocate_queue_buffer();
				m_syncFlimProcessing.deallocate_queue_buffer();

				// Reset Widgets /////////////////////////////////////////////////////////////////////////////
				setWidgets(true, m_pConfigTemp);
				m_pResultTab->getVisualizationTab()->setWidgets(true, m_pConfigTemp);

				// Visualization /////////////////////////////////////////////////////////////////////////////
				m_pResultTab->getVisualizationTab()->visualizeEnFaceMap(true);
				m_pResultTab->getVisualizationTab()->visualizeImage(0);
			}

			///std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start;
			///printf("elapsed time : %.2f sec\n\n", elapsed.count());
		});

		t1.detach();
	}
}


void QProcessingTab::loadingRawData(QFile* pFile, Configuration* pConfig)
{
	int frameCount = 0;
	while (frameCount < pConfig->frames + pConfig->interFrameSync)
	{
		// Get buffers from threading queues
		uint8_t* frame_data = nullptr;
		do
		{
			{
				std::unique_lock<std::mutex> lock(m_syncDeinterleaving.mtx);
				if (!m_syncDeinterleaving.queue_buffer.empty())
				{
					frame_data = m_syncDeinterleaving.queue_buffer.front();
					m_syncDeinterleaving.queue_buffer.pop();
				}
			}

			if (frame_data)
			{
				// Read data from the external data
				pFile->read(reinterpret_cast<char *>(frame_data), sizeof(uint16_t) * pConfig->flimFrameSize + sizeof(uint8_t) * pConfig->octFrameSize);
				frameCount++;

				// Push the buffers to sync Queues
				m_syncDeinterleaving.Queue_sync.push(frame_data);
			}
		} while (frame_data == nullptr);
	}
}

void QProcessingTab::deinterleaving(Configuration* pConfig)
{
	QVisualizationTab* pVisTab = m_pResultTab->getVisualizationTab();

	int frameCount = 0;
	while (frameCount < pConfig->frames + pConfig->interFrameSync)
	{
		// Get the buffer from the previous sync Queue
		uint8_t* frame_ptr = m_syncDeinterleaving.Queue_sync.pop();
		if (frame_ptr != nullptr)
		{
			// Get buffers from threading queues
			uint16_t* pulse_ptr = nullptr;
			do
			{
				if (pulse_ptr == nullptr)
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
					// Data deinterleaving
					memcpy(pulse_ptr, frame_ptr, sizeof(uint16_t) * pConfig->flimFrameSize);
					if (frameCount >= pConfig->interFrameSync)
						memcpy(pVisTab->m_vectorOctImage.at(frameCount - pConfig->interFrameSync).raw_ptr(),
							frame_ptr + sizeof(uint16_t) * pConfig->flimFrameSize, sizeof(uint8_t) * pConfig->octFrameSize);
					frameCount++;

					// Push the buffers to sync Queues
					m_syncFlimProcessing.Queue_sync.push(pulse_ptr);

					// Return (push) the buffer to the previous threading queue
					{
						std::unique_lock<std::mutex> lock(m_syncDeinterleaving.mtx);
						m_syncDeinterleaving.queue_buffer.push(frame_ptr);
					}
				}
			} while (pulse_ptr == nullptr);
		}
		else
		{
			printf("deinterleaving is halted.\n");
			break;
		}
	}
}

void QProcessingTab::flimProcessing(FLImProcess* pFLIm, Configuration* pConfig)
{
	QVisualizationTab* pVisTab = m_pResultTab->getVisualizationTab();

	np::Array<float, 2> itn(pConfig->flimAlines, 4); // temp intensity
	np::Array<float, 2> md(pConfig->flimAlines, 4); // temp mean delay
	np::Array<float, 2> ltm(pConfig->flimAlines, 3); // temp lifetime
	
	///QFile file("pulse.data");
	///file.open(QIODevice::WriteOnly);

	int frameCount = 0;
	while (frameCount < pConfig->frames + pConfig->interFrameSync)
	{
		// Get the buffer from the previous sync Queue
		uint16_t* pulse_data = m_syncFlimProcessing.Queue_sync.pop();
		if (pulse_data != nullptr)
		{
			if (frameCount < pConfig->frames)
			{
				// Pulse array definition
				np::Uint16Array2 pulse_temp(pulse_data, pConfig->flimScans, pConfig->flimAlines);
				np::Uint16Array2 pulse(pConfig->flimScans, pConfig->flimAlines);

				// Additional intra frame synchronization process by pulse circulating
				memcpy(&pulse(0, 0), &pulse_temp(0, pConfig->intraFrameSync), sizeof(uint16_t) * pulse.size(0) * (pulse.size(1) - pConfig->intraFrameSync));
				memcpy(&pulse(0, pulse.size(1) - pConfig->intraFrameSync), &pulse_temp(0, 0), sizeof(uint16_t) * pulse.size(0) * pConfig->intraFrameSync);

				// FLIM Process
				(*pFLIm)(itn, md, ltm, pulse);

				// Copy for Pulse Review
				np::Array<float, 2> crop(pFLIm->_resize.nx, pFLIm->_resize.ny);
				np::Array<float, 2> mask(pFLIm->_resize.nx, pFLIm->_resize.ny);
				memcpy(crop, pFLIm->_resize.crop_src, crop.length() * sizeof(float));
				memcpy(mask, pFLIm->_resize.mask_src, mask.length() * sizeof(float));
				pVisTab->m_vectorPulseCrop.push_back(crop);
				pVisTab->m_vectorPulseMask.push_back(mask);
				
				// Intensity compensation
				for (int i = 0; i < 3; i++)
					ippsDivC_32f_I(pConfig->flimIntensityComp[i], &itn(0, i + 1), pConfig->flimAlines);

				///file.write(reinterpret_cast<const char*>(pFLIm->_resize.filt_src.raw_ptr()), sizeof(float) * pFLIm->_resize.nsite);

				// Copy for Intensity & Lifetime			
				memcpy(&pVisTab->m_intensityMap.at(0)(0, frameCount), &itn(0, 1), sizeof(float) * pConfig->flimAlines);
				memcpy(&pVisTab->m_intensityMap.at(1)(0, frameCount), &itn(0, 2), sizeof(float) * pConfig->flimAlines);
				memcpy(&pVisTab->m_intensityMap.at(2)(0, frameCount), &itn(0, 3), sizeof(float) * pConfig->flimAlines);
				memcpy(&pVisTab->m_lifetimeMap.at(0)(0, frameCount), &ltm(0, 0), sizeof(float) * pConfig->flimAlines);
				memcpy(&pVisTab->m_lifetimeMap.at(1)(0, frameCount), &ltm(0, 1), sizeof(float) * pConfig->flimAlines);
				memcpy(&pVisTab->m_lifetimeMap.at(2)(0, frameCount), &ltm(0, 2), sizeof(float) * pConfig->flimAlines);
			}
			emit processedSingleFrame(frameCount++);

			// Return (push) the buffer to the previous threading queue
			{
				std::unique_lock<std::mutex> lock(m_syncFlimProcessing.mtx);
				m_syncFlimProcessing.queue_buffer.push(pulse_data);
			}
		}
		else
		{
			printf("flimProcessing is halted.\n");
			break;
		}
	}

	///file.close();
}


void QProcessingTab::getOctProjection(std::vector<np::Uint8Array2>& vecImg, np::Uint8Array2& octProj, int offset)
{
	int len = vecImg.at(0).size(1);
	tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)vecImg.size()),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
			uint8_t maxVal, minVal;
			for (int j = 0; j < octProj.size(0); j++)
			{
				ippsMinMax_8u(&vecImg.at((int)i)(offset + OUTER_SHEATH_POSITION, j), len - OUTER_SHEATH_POSITION, &minVal, &maxVal);
				octProj(j, (int)i) = maxVal;
			}
		}
	});
}