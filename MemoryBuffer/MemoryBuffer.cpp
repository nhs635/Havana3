
#include "MemoryBuffer.h"

#include <Havana3/Configuration.h>
#include <Havana3/QOperationTab.h>
#include <Havana3/MainWindow.h>
#include <Havana3/QStreamTab.h>
#include <Havana3/QDeviceControlTab.h>

#include <DeviceControl/ZaberStage/ZaberStage.h>

#include <iostream>
#include <deque>
#include <chrono>
#include <mutex>
#include <condition_variable>


MemoryBuffer::MemoryBuffer(QObject *parent) :
    QObject(parent),
	m_bIsAllocatedWritingBuffer(false), 
	m_bIsRecording(false), m_bIsSaved(false),
	m_nRecordedFrames(0)
{
	m_pOperationTab = (QOperationTab*)parent;
	m_pConfig = m_pOperationTab->getStreamTab()->getMainWnd()->m_pConfiguration;
}

MemoryBuffer::~MemoryBuffer()
{
	for (int i = 0; i < WRITING_BUFFER_SIZE; i++)
	{
		if (!m_queueWritingFlimBuffer.empty())
		{
			uint16_t* buffer = m_queueWritingFlimBuffer.front();
			if (buffer)
			{
				m_queueWritingFlimBuffer.pop();
				delete[] buffer;
			}
		}

		if (!m_queueWritingOctBuffer.empty())
		{
			uint8_t* buffer = m_queueWritingOctBuffer.front();
			if (buffer)
			{
				m_queueWritingOctBuffer.pop();
				delete[] buffer;
			}
		}
	}
	SendStatusMessage("Writing buffers are successfully disallocated.", false);
}


void MemoryBuffer::allocateWritingBuffer()
{
	if (!m_bIsAllocatedWritingBuffer)
	{
		for (int i = 0; i < WRITING_BUFFER_SIZE; i++)
		{
			uint16_t* buffer1 = new uint16_t[m_pConfig->flimFrameSize];
			memset(buffer1, 0, m_pConfig->flimFrameSize * sizeof(uint16_t));
			m_queueWritingFlimBuffer.push(buffer1);

			uint8_t* buffer2 = new uint8_t[m_pConfig->octFrameSize];
			memset(buffer2, 0, m_pConfig->octFrameSize * sizeof(uint8_t));
			m_queueWritingOctBuffer.push(buffer2);
			
			//printf("\rAllocating the writing buffers... [%d / %d]", i + 1, WRITING_BUFFER_SIZE);
		}

		m_syncFlimBuffering.allocate_queue_buffer(m_pConfig->flimScans, m_pConfig->flimAlines, PROCESSING_BUFFER_SIZE);
		m_syncOctBuffering.allocate_queue_buffer(m_pConfig->octScans, m_pConfig->octAlines, PROCESSING_BUFFER_SIZE);
		
		char msg[256];
		sprintf(msg, "Writing buffers are successfully allocated. [Number of buffers: %d]", WRITING_BUFFER_SIZE);
		SendStatusMessage(msg, false); 
		SendStatusMessage("Now, recording process is available!", false);

		m_bIsAllocatedWritingBuffer = true;
	}

	emit finishedBufferAllocation();
}

bool MemoryBuffer::startRecording()
{
	// Check if the previous recorded data is saved
	if (!m_bIsSaved && (m_nRecordedFrames != 0))
	{
		QMessageBox MsgBox;
		MsgBox.setWindowTitle("Warning");
		MsgBox.setIcon(QMessageBox::Warning);
		MsgBox.setText("The previous recorded data is not saved.\nWhat would you like to do with this data?");
		MsgBox.setStandardButtons(QMessageBox::Ignore | QMessageBox::Discard | QMessageBox::Cancel);
		MsgBox.setDefaultButton(QMessageBox::Cancel);

		int resp = MsgBox.exec();
		switch (resp)
		{
		case QMessageBox::Ignore:
			break;
		case QMessageBox::Discard:
			m_bIsSaved = true;
			m_nRecordedFrames = 0;
			return false;
		case QMessageBox::Cancel:
			return false;
		default:
			break;
		}
	}

	// Start Recording
	SendStatusMessage("Data recording is started.", false);
	m_nRecordedFrames = 0;

    // Pullback
	m_pDeviceControlTab = m_pOperationTab->getStreamTab()->getDeviceControlTab();
    if (m_pDeviceControlTab->isZaberStageEnabled())
    {
        m_pDeviceControlTab->getZaberStage()->DidMovedAbsolute += [&]() {
            // Finish recording when the pullback is finished.
            m_bIsRecording = false;
            m_pOperationTab->setRecordingButton(false);
        };
        m_pDeviceControlTab->pullback();
    }
	
	// Start Recording
	m_bIsRecording = true;
	m_bIsSaved = false;

	// Thread for buffering transfered data (memcpy)
	std::thread	thread_buffering_data = std::thread([&]() {
		SendStatusMessage("Data buffering thread is started.", false);
		while (1)
		{
			// Get the buffer from the buffering sync Queue
			uint16_t* pulse = m_syncFlimBuffering.Queue_sync.pop();
			uint8_t* oct_im = m_syncOctBuffering.Queue_sync.pop();
			if ((pulse != nullptr) && (oct_im != nullptr))
			{
				// Body
				if (m_nRecordedFrames < WRITING_BUFFER_SIZE)
				{
					uint16_t* buffer_flim = m_queueWritingFlimBuffer.front();
					m_queueWritingFlimBuffer.pop();
					memcpy(buffer_flim, pulse, sizeof(uint16_t) * m_pConfig->flimFrameSize);
					m_queueWritingFlimBuffer.push(buffer_flim);

					uint8_t* buffer_oct = m_queueWritingOctBuffer.front();
					m_queueWritingOctBuffer.pop();
					memcpy(buffer_oct, oct_im, sizeof(uint8_t) * m_pConfig->octFrameSize);
					m_queueWritingOctBuffer.push(buffer_oct);

					m_nRecordedFrames++;
					///printf("ing: %zd %zd\n", m_syncFlimBuffering.Queue_sync.size(), m_syncOctBuffering.Queue_sync.size());
					
					// Return (push) the buffer to the buffering threading queue
					{
						std::unique_lock<std::mutex> lock(m_syncFlimBuffering.mtx);
						m_syncFlimBuffering.queue_buffer.push(pulse);
					}
					{
						std::unique_lock<std::mutex> lock(m_syncOctBuffering.mtx);
						m_syncOctBuffering.queue_buffer.push(oct_im);
					}
				}
			}
			else
			{
				printf("end: %zd %zd\n", m_syncFlimBuffering.Queue_sync.size(), m_syncOctBuffering.Queue_sync.size());
				printf("who null: %d %d\n", (pulse == nullptr), (oct_im == nullptr));

				if (pulse != nullptr)
				{
					uint16_t* pulse_temp = pulse;
					do
					{
						m_syncFlimBuffering.queue_buffer.push(pulse_temp);
						pulse_temp = m_syncFlimBuffering.Queue_sync.pop();
					} while (pulse_temp != nullptr);
				}
				if (oct_im != nullptr)
				{
					uint8_t* oct_temp = oct_im;
					do
					{
						m_syncOctBuffering.queue_buffer.push(oct_temp);
						oct_temp = m_syncOctBuffering.Queue_sync.pop();
					} while (oct_temp != nullptr);
				}

				break; ///////////////////////////////////////////////////////////////////////////////////////////////// ?
			}
		}
		SendStatusMessage("Data copying thread is finished.", false);
	});
	thread_buffering_data.detach();

	return true;
}

void MemoryBuffer::stopRecording()
{
	// Stop recording
	m_bIsRecording = false;
		
	if (m_nRecordedFrames != 0) // Not allowed when 'discard'
	{
		// Push nullptr to Buffering Queue
		m_syncFlimBuffering.Queue_sync.push(nullptr);
		m_syncOctBuffering.Queue_sync.push(nullptr);

		// Status update
		m_pConfig->frames = m_nRecordedFrames;
		uint64_t total_size = (uint64_t)m_nRecordedFrames * (uint64_t)(m_pConfig->flimFrameSize * sizeof(uint16_t) + m_pConfig->octFrameSize * sizeof(uint8_t)) / (uint64_t)1024;

		char msg[256];
		sprintf(msg, "Data recording is finished normally. \n(Recorded frames: %d frames (%.2f MB)", m_nRecordedFrames, (double)total_size / 1024.0);
		SendStatusMessage(msg, false);
	}
}

bool MemoryBuffer::startSaving()
{
	// Get path to write
	m_fileName = QFileDialog::getSaveFileName(nullptr, "Save As", "", "FLIm OCT raw data (*.data)");
	if (m_fileName == "") return false;
	
	// Start writing thread
	std::thread _thread = std::thread(&MemoryBuffer::write, this);
	_thread.detach();

	return true;
}

//void MemoryBuffer::circulation(int nFramesToCirc)
//{
//	for (int i = 0; i < nFramesToCirc; i++)
//	{
//		uint16_t* buffer = m_queueWritingBuffer.front();
//		m_queueWritingBuffer.pop();
//		m_queueWritingBuffer.push(buffer);
//	}
//}
//
//uint16_t* MemoryBuffer::pop_front()
//{
//	uint16_t* buffer = m_queueWritingBuffer.front();
//	m_queueWritingBuffer.pop();
//
//	return buffer;
//}
//
//void MemoryBuffer::push_back(uint16_t* buffer)
//{
//	m_queueWritingBuffer.push(buffer);
//}


void MemoryBuffer::write()
{	
	qint64 res;
	qint64 flimSamplesToWrite;
	qint64 octSamplesToWrite = m_pConfig->octFrameSize;

	if (QFile::exists(m_fileName))
	{
		SendStatusMessage("Havana3 does not overwrite a recorded data.", false);
		emit finishedWritingThread(true);
		return;
	}

	// Move to start point
	uint16_t* buffer_flim = nullptr;
	uint8_t* buffer_oct = nullptr;
	for (int i = 0; i < WRITING_BUFFER_SIZE - m_nRecordedFrames; i++)
	{
		buffer_flim = m_queueWritingFlimBuffer.front();
		m_queueWritingFlimBuffer.pop();
		m_queueWritingFlimBuffer.push(buffer_flim);

		buffer_oct = m_queueWritingOctBuffer.front();
		m_queueWritingOctBuffer.pop();
		m_queueWritingOctBuffer.push(buffer_oct);
	}

	// Writing
	QFile file(m_fileName);
	if (file.open(QIODevice::WriteOnly))
	{
		for (int i = 0; i < m_nRecordedFrames; i++)
		{
			// FLIm pulse writing
			buffer_flim = m_queueWritingFlimBuffer.front();
			m_queueWritingFlimBuffer.pop();			
			flimSamplesToWrite = m_pConfig->flimScans * (m_pConfig->flimAlines - INTRA_FRAME_SYNC);
			res = file.write(reinterpret_cast<char*>(buffer_flim + m_pConfig->flimScans * INTRA_FRAME_SYNC), sizeof(uint16_t) * flimSamplesToWrite);
			if (!(res == sizeof(uint16_t) * flimSamplesToWrite))
			{
				SendStatusMessage("Error occurred while writing...", true);
				emit finishedWritingThread(true);
				return;
			}	
			flimSamplesToWrite = m_pConfig->flimScans * INTRA_FRAME_SYNC;
			res = file.write(reinterpret_cast<char*>(buffer_flim), sizeof(uint16_t) * flimSamplesToWrite);
			if (!(res == sizeof(uint16_t) * flimSamplesToWrite))
			{
				SendStatusMessage("Error occurred while writing...", true);
				emit finishedWritingThread(true);
				return;
			}
			m_queueWritingFlimBuffer.push(buffer_flim);

			// OCT image writing
			buffer_oct = m_queueWritingOctBuffer.front();
			m_queueWritingOctBuffer.pop();
			res = file.write(reinterpret_cast<char*>(buffer_oct), sizeof(uint8_t) * octSamplesToWrite);
			if (!(res == sizeof(uint8_t) * octSamplesToWrite))
			{
				SendStatusMessage("Error occurred while writing...", true);
				emit finishedWritingThread(true);
				return;
			}		
			m_queueWritingOctBuffer.push(buffer_oct);

			emit wroteSingleFrame(i);
			//printf("\r%dth frame is wrote... [%3.2f%%]", i + 1, 100 * (double)(i + 1) / (double)m_nRecordedFrames);
		}
		file.close();
	}
	else
	{
		SendStatusMessage("Error occurred during writing process.", true);
		return;
	}
	m_bIsSaved = true;

	// Move files
	QString fileTitle, filePath;
	for (int i = 0; i < m_fileName.length(); i++)
	{
		if (m_fileName.at(i) == QChar('.')) fileTitle = m_fileName.left(i);
		if (m_fileName.at(i) == QChar('/')) filePath = m_fileName.left(i);
	}

	m_pConfig->setConfigFile("Havana3.ini");
	if (false == QFile::copy("Havana3.ini", fileTitle + ".ini"))
		SendStatusMessage("Error occurred while copying configuration data.", true);

	if (false == QFile::copy("flim_mask.dat", fileTitle + ".flim_mask"))
		SendStatusMessage("Error occurred while copying flim_mask data.", true);

	if (false == QFile::copy("Havana3.m", fileTitle + ".m"))
		printf("Error occurred while copying MATLAB processing data.\n");
	
	// Send a signal to notify this thread is finished
	emit finishedWritingThread(false);

	// Status update
	char msg[256];
	sprintf(msg, "Data saving thread is finished normally. (Saved frames: %d frames)", m_nRecordedFrames);
	SendStatusMessage(msg, false);

	QByteArray temp = m_fileName.toLocal8Bit();
	char* filename = temp.data();
	sprintf(msg, "[%s]", filename);
	SendStatusMessage(msg, false);
}
