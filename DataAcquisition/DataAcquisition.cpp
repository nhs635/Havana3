
#include "DataAcquisition.h"

#include <QDebug>
#include <QMessageBox>

#include <Havana3/Configuration.h>

#ifndef NEXT_GEN_SYSTEM
#include <DataAcquisition/SignatecDAQ/SignatecDAQ.h>
#ifdef AXSUN_ENABLE
#include <DataAcquisition/AxsunCapture/AxsunCapture.h>
#endif
#else
#include <DataAcquisition/AlazarDAQ/AlazarDAQ.h>
#endif
#include <DataAcquisition/FLImProcess/FLImProcess.h>


DataAcquisition::DataAcquisition(Configuration* pConfig)
    : m_bAcquisitionState(false), m_bIsPaused(false), 
	m_pAxsunCapture(nullptr), m_pDaqOct(nullptr), m_pDaqFlim(nullptr),
	m_pDaq(nullptr), m_pFLIm(nullptr)
{
    // Set main window objects
    m_pConfig = pConfig;

	// Message & error handling function object
	auto messgae_handling = [&](const char* msg, bool is_error) {
		QString qmsg = QString::fromUtf8(msg);
		if (is_error)
		{
			m_pConfig->writeToLog(QString("[ERROR] %1").arg(qmsg));
			QMessageBox MsgBox(QMessageBox::Critical, "Data Acquisition Error", qmsg);
			MsgBox.exec();
		}
		else
		{
			m_pConfig->writeToLog(qmsg);
			qDebug() << qmsg;			
		}
	};
	
#ifndef NEXT_GEN_SYSTEM
#ifdef AXSUN_ENABLE
	// Create Axsun OCT capture object
	m_pAxsunCapture = new AxsunCapture;
	m_pAxsunCapture->SendStatusMessage += messgae_handling;
	m_pAxsunCapture->DidStopData += [&]() { m_pAxsunCapture->capture_running = false; };
#endif

    // Create SignatecDAQ object
    m_pDaq = new SignatecDAQ;
	m_pDaq->SendStatusMessage += messgae_handling;
    m_pDaq->DidStopData += [&]() { m_pDaq->_running = false; };
#else
	// Create AlazarDAQ object
	m_pDaqOct = new AlazarDAQ;
	m_pDaqOct->SendStatusMessage += messgae_handling;
	m_pDaqOct->DidStopData += [&]() { m_pDaqOct->_running = false; };

	m_pDaqFlim = new AlazarDAQ;
	m_pDaqFlim->SendStatusMessage += messgae_handling;
	m_pDaqFlim->DidStopData += [&]() { m_pDaqFlim->_running = false; };
#endif

    // Create FLIm process object
    m_pFLIm = new FLImProcess;
	m_pFLIm->SendStatusMessage += messgae_handling;
	///m_pFLIm->_resize.SendStatusMessage += [&](const char* msg) { m_pConfig->msgHandle(msg); };
    m_pFLIm->setParameters(m_pConfig);
    m_pFLIm->_resize(np::Uint16Array2(m_pConfig->flimScans, m_pConfig->flimAlines), m_pFLIm->_params);
    m_pFLIm->loadMaskData();
}

DataAcquisition::~DataAcquisition()
{
#ifndef NEXT_GEN_SYSTEM
#ifdef AXSUN_ENABLE
	if (m_pAxsunCapture) delete m_pAxsunCapture;
#endif
    if (m_pDaq) delete m_pDaq;
#else
	if (m_pDaqOct) delete m_pDaqOct;
	if (m_pDaqFlim) delete m_pDaqFlim;
#endif
	if (m_pFLIm)
	{
		m_pFLIm->saveMaskData();
		delete m_pFLIm;
	}
}


bool DataAcquisition::InitializeAcquistion()
{
#ifndef NEXT_GEN_SYSTEM
    /// Set boot-time buffer
    ///SetBootTimeBufCfg(PX14_BOOTBUF_IDX, sizeof(uint16_t) * m_pConfig->flimScans * m_pConfig->flimAlines);

    // Parameter settings for DAQ & Axsun Capture
#ifdef AXSUN_ENABLE
	m_pAxsunCapture->image_height = m_pConfig->octScans;
	m_pAxsunCapture->image_width = m_pConfig->octAlines;
#endif

    m_pDaq->nScans = m_pConfig->flimScans;
    m_pDaq->nAlines = m_pConfig->flimAlines;
    m_pDaq->BootTimeBufIdx = PX14_BOOTBUF_IDX;
	
    // Initialization for DAQ & Axsun Capture
#ifdef AXSUN_ENABLE
    if (!m_pDaq->set_init() || !m_pAxsunCapture->initializeCapture())	
#else
	if (!m_pDaq->set_init())
#endif
    {
		StopAcquisition();

		m_pConfig->writeToLog("Data acq initialization failed.");
        return false;
    }
#else
	// Parameter settings for DAQ
	m_pDaqOct->SystemId = 1;  // ATS9371 (OCT)		
	//m_pDaqOct->AcqRate = SAMPLE_RATE_500MSPS;
	m_pDaqOct->nChannels = 1;
	m_pDaqOct->nScans = m_pConfig->octScans;
	m_pDaqOct->nScansFFT = m_pConfig->octScansFFT;
	m_pDaqOct->nAlines = m_pConfig->octAlines;
	m_pDaqOct->TriggerDelay = 0;
	m_pDaqOct->VoltRange1 = INPUT_RANGE_PM_400_MV;
	m_pDaqOct->VoltRange2 = INPUT_RANGE_PM_400_MV;
	m_pDaqOct->TriggerSlope = TRIGGER_SLOPE_POSITIVE;
	m_pDaqOct->UseExternalClock = true;  // k-clock
#ifdef ENABLE_FPGA_FFT
	m_pDaqOct->UseFFTModule = true;  // on-board fpga fft processing
	m_pDaqOct->SetBackground += [&]() {
		m_pDaqOct->background = np::Array<int16_t>(OCT_SCANS);
		QFile bg_file("bg.bin");
		if (bg_file.open(QIODevice::ReadOnly))
			bg_file.read(reinterpret_cast<char*>(m_pDaqOct->background.raw_ptr()), sizeof(int16_t) * m_pDaqOct->background.length());
		bg_file.close();
	};
#else
	m_pDaqOct->UseFFTModule = false;
#endif

	m_pDaqFlim->SystemId = 2;  // ATS9352 (FLIM)
	m_pDaqFlim->AcqRate = SAMPLE_RATE_500MSPS;
	m_pDaqFlim->nChannels = 1;
	m_pDaqFlim->nScans = m_pConfig->flimScans;
	m_pDaqFlim->nAlines = m_pConfig->flimAlines;
	m_pDaqFlim->TriggerDelay = 0;
	m_pDaqFlim->VoltRange1 = INPUT_RANGE_PM_2_V;
	m_pDaqFlim->VoltRange2 = INPUT_RANGE_PM_2_V;
	m_pDaqFlim->TriggerSlope = TRIGGER_SLOPE_POSITIVE;

	// Initialization for DAQ & Axsun Capture
	if (!m_pDaqOct->set_init() || !m_pDaqFlim->set_init())
	{
		StopAcquisition();

		m_pConfig->writeToLog("Data acq initialization failed.");
		return false;
	}
#endif

	m_pConfig->writeToLog("Data acq initialization successed.");

    return true;
}

bool DataAcquisition::StartAcquisition()
{
#ifndef NEXT_GEN_SYSTEM
    // Parameter settings for DAQ
    m_pDaq->DcOffset = m_pConfig->px14DcOffset;

    // Start acquisition
#ifdef AXSUN_ENABLE
	if (!m_pDaq->startAcquisition() || !m_pAxsunCapture->startCapture())
#else
	if (!m_pDaq->startAcquisition())
#endif
	{
		StopAcquisition();

		m_pConfig->writeToLog("Data acq failed.");
		return false;
	}
#else
	// Start acquisition
	if (!m_pDaqOct->startAcquisition() || !m_pDaqFlim->startAcquisition())
	{
		StopAcquisition();

		m_pConfig->writeToLog("Data acq failed.");
		return false;
	}
#endif

	m_bAcquisitionState = true;
	m_bIsPaused = false;

	m_pConfig->writeToLog("Data acq successed.");

    return true;
}

void DataAcquisition::StopAcquisition()
{
    // Stop thread
	if (m_bAcquisitionState)
	{
#ifndef NEXT_GEN_SYSTEM
		m_pDaq->stopAcquisition();
#ifdef AXSUN_ENABLE
		m_pAxsunCapture->stopCapture();
#endif
#else
		m_pDaqOct->stopAcquisition();
		m_pDaqFlim->stopAcquisition();
#endif

		m_pConfig->writeToLog("Data acq stopped.");
	}

	m_bIsPaused = m_bAcquisitionState;
	m_bAcquisitionState = false;	
}


void DataAcquisition::GetBootTimeBufCfg(int idx, int& buffer_size)
{
#ifndef NEXT_GEN_SYSTEM
    buffer_size = m_pDaq->getBootTimeBuffer(idx);
#else
	(void)idx;
	(void)buffer_size;
#endif
}

void DataAcquisition::SetBootTimeBufCfg(int idx, int buffer_size)
{
#ifndef NEXT_GEN_SYSTEM
    m_pDaq->setBootTimeBuffer(idx, buffer_size);
#else
	(void)idx;
	(void)buffer_size;
#endif
}

void DataAcquisition::SetDcOffset(int offset)
{
#ifndef NEXT_GEN_SYSTEM
    m_pDaq->setDcOffset(offset);
#else
	(void)offset;
#endif
}


void DataAcquisition::ConnectAcquiredFlimData(const std::function<void(int, const np::Array<uint16_t, 2>&)> &slot)
{
#ifndef NEXT_GEN_SYSTEM
    m_pDaq->DidAcquireData += slot;
#else
	(void)slot;
#endif
}

void DataAcquisition::ConnectAcquiredFlimData1(const std::function<void(int, const void*)> &slot)
{
#ifdef NEXT_GEN_SYSTEM
	m_pDaqFlim->DidAcquireData += slot;
#else
	(void)slot;
#endif
}

void DataAcquisition::ConnectStopFlimData(const std::function<void(void)> &slot)
{
#ifndef NEXT_GEN_SYSTEM
    m_pDaq->DidStopData += slot;
#else
	m_pDaqFlim->DidStopData += slot;
#endif
}

void DataAcquisition::ConnectFlimSendStatusMessage(const std::function<void(const char*, bool)> &slot)
{
#ifndef NEXT_GEN_SYSTEM
    m_pDaq->SendStatusMessage += slot;
#else
	m_pDaqFlim->SendStatusMessage += slot;
#endif
}

void DataAcquisition::ConnectAcquiredOctData(const std::function<void(uint32_t, const np::Uint8Array2&)> &slot)
{
#ifdef AXSUN_ENABLE
#ifndef NEXT_GEN_SYSTEM
	m_pAxsunCapture->DidAcquireData += slot;
#else
	(void)slot;
#endif
#else
	(void)slot;
#endif
}

void DataAcquisition::ConnectAcquiredOctBG(const std::function<void(uint32_t, const np::Uint8Array2&)> &slot)
{
#ifdef AXSUN_ENABLE
#ifndef NEXT_GEN_SYSTEM
	m_pAxsunCapture->DidAcquireBG += slot;
#else
	(void)slot;
#endif
#else
	(void)slot;
#endif
}

void DataAcquisition::ConnectAcquiredOctData1(const std::function<void(int, const void*)> &slot)
{
#ifdef NEXT_GEN_SYSTEM
	m_pDaqOct->DidAcquireData += slot;
#else
	(void)slot;
#endif
}

void DataAcquisition::ConnectStopOctData(const std::function<void(void)> &slot)
{
#ifdef AXSUN_ENABLE
#ifndef NEXT_GEN_SYSTEM
	m_pAxsunCapture->DidStopData += slot;
#else
	m_pDaqOct->DidStopData += slot;
#endif
#else
(void)slot;
#endif
}

void DataAcquisition::ConnectOctSendStatusMessage(const std::function<void(const char*, bool)> &slot)
{
#ifdef AXSUN_ENABLE
#ifndef NEXT_GEN_SYSTEM
	m_pAxsunCapture->SendStatusMessage += slot;
#else
	m_pDaqOct->SendStatusMessage += slot;
#endif
#else
(void)slot;
#endif
}
