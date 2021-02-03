
#include "AlazarDAQ.h"


using namespace std;

AlazarDAQ::AlazarDAQ() :
    SystemId(1), nChannels(1), nScans(1000), nScansFFT(1024), nAlines(1024),
    VoltRange1(INPUT_RANGE_PM_400_MV), VoltRange2(INPUT_RANGE_PM_400_MV),
    AcqRate(SAMPLE_RATE_500MSPS), TriggerDelay(0), TriggerSlope(TRIGGER_SLOPE_POSITIVE),
    UseExternalClock(false), UseAutoTrigger(false), UseFFTModule(false), frameRate(0.0),
    _dirty(true), _running(false), boardHandle(nullptr), fftHandle(nullptr)
{
}

AlazarDAQ::~AlazarDAQ()
{
    if (_thread.joinable())
    {
        _running = false;
        _thread.join();
    }

    // Abort the acquisition
    abort_acq();
}


bool AlazarDAQ::initialize()
{    
    RETURN_CODE retCode = ApiSuccess;

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Select a board ///////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////
    U32 systemId = SystemId;
    U32 boardId = 1;

	// Get a handle to the board
	boardHandle = AlazarGetBoardBySystemID(systemId, boardId);
    if (boardHandle == nullptr)
	{
        dumpError(retCode, "[AlazarDAQ] Error: Unable to open board system Id & board Id");
		return false;
	}

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Specify the sample rate (see sample rate id below) ///////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////

    // double samplesPerSec = 500.e6;
    const U32 SamplingRate = AcqRate;  // SAMPLE_RATE_500MSPS; // only for internal clock

	// Select clock parameters as required to generate this sample rate.
	//
	// For example: if samplesPerSec is 100.e6 (100 MS/s), then:
	// - select clock source INTERNAL_CLOCK and sample rate SAMPLE_RATE_100MSPS
	// - select clock source FAST_EXTERNAL_CLOCK, sample rate SAMPLE_RATE_USER_DEF,
	//   and connect a 100 MHz signalto the EXT CLK BNC connector.
	retCode = 
		AlazarSetCaptureClock(
			boardHandle,			// HANDLE -- board handle
            UseExternalClock ? EXTERNAL_CLOCK_AC : INTERNAL_CLOCK,		// U32 -- clock source id
            UseExternalClock ? SAMPLE_RATE_USER_DEF : SamplingRate,	// U32 -- sample rate id
			CLOCK_EDGE_RISING,		// U32 -- clock edge id
			0						// U32 -- clock decimation 
			);
	if (retCode != ApiSuccess)
	{
        dumpError(retCode, "[AlazarDAQ] Error: AlazarSetCaptureClock failed: ");
		return false;
	}

    // External Clock Settings
    if (UseExternalClock)
    {
        retCode =
            AlazarSetExternalClockLevel(
            boardHandle,			// HANDLE -- board handle
            30.f                    // Voltage level in percentage
            );
        if (retCode != ApiSuccess)
        {
            dumpError(retCode, "[AlazarDAQ] Error: AlazarSetExternalClockLevel failed: ");
            return false;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Input channel setting ////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////

	// Select CHA input parameters as required
	retCode = 
		AlazarInputControl(
			boardHandle,			// HANDLE -- board handle
			CHANNEL_A,			    // U8 -- input channel 
            DC_COUPLING,			// U32 -- input coupling id
            VoltRange1,     		// U32 -- input range id
			IMPEDANCE_50_OHM		// U32 -- input impedance id
			);
	if (retCode != ApiSuccess)
	{
        dumpError(retCode, "[AlazarDAQ] Error: AlazarInputControl failed: ");
		return false;
    }
	
	// Select CHB input parameters as required
	retCode = 
		AlazarInputControl(
			boardHandle,			// HANDLE -- board handle
			CHANNEL_B,				// U8 -- channel identifier
            DC_COUPLING,			// U32 -- input coupling id
            VoltRange2,         	// U32 -- input range id
            IMPEDANCE_50_OHM		// U32 -- input impedance id
			);
	if (retCode != ApiSuccess)
	{
        dumpError(retCode, "[AlazarDAQ] Error: AlazarInputControl failed: ");
		return false;
	}

	// Select trigger inputs and levels as required
	retCode =
		AlazarSetTriggerOperation(
			boardHandle,			// HANDLE -- board handle
			TRIG_ENGINE_OP_J,		// U32 -- trigger operation
			TRIG_ENGINE_J,			// U32 -- trigger engine id
			TRIG_EXTERNAL,			// U32 -- trigger source id
			TRIGGER_SLOPE_POSITIVE,	// U32 -- trigger slope id
			150,					// Utrigger32 -- trigger level from 0 (-range) to 255 (+range)
			TRIG_ENGINE_K,			// U32 -- trigger engine id
			TRIG_DISABLE,			// U32 -- trigger source id for engine K
			TRIGGER_SLOPE_POSITIVE,	// U32 -- trigger slope id
			128						// U32 -- trigger level from 0 (-range) to 255 (+range)
		);
	if (retCode != ApiSuccess)
	{
		dumpError(retCode, "[AlazarDAQ] Error: AlazarSetTriggerOperation failed: ");
		return false;
	}

	// Select external trigger parameters as required
	retCode =
		AlazarSetExternalTrigger(
			boardHandle,			// HANDLE -- board handle
			DC_COUPLING,			// U32 -- external trigger coupling id
			ETR_TTL					// U32 -- external trigger range id
		);
	if (retCode != ApiSuccess)
	{
		dumpError(retCode, "[AlazarDAQ] Error: AlazarSetExternalTrigger failed: ");
		return false;
	}

	// Set trigger delay as required.
	retCode = AlazarSetTriggerDelay(boardHandle, 0);
	if (retCode != ApiSuccess)
	{
		dumpError(retCode, "[AlazarDAQ] Error: AlazarSetTriggerDelay failed: ");
		return false;
	}

    // Set trigger timeout as required.
    // NOTE:
    // The board will wait for a for this amount of time for a trigger event.
    // If a trigger event does not arrive, then the board will automatically
    // trigger. Set the trigger timeout value to 0 to force the board to wait
    // forever for a trigger event.
    //
    // IMPORTANT:
    // The trigger timeout value should be set to zero after appropriate
    // trigger parameters have been determined, otherwise the
    // board may trigger if the timeout interval expires before a
    // hardware trigger event arrives.

    //double triggerTimeout_sec = 0.;
    //U32 triggerTimeout_clocks = (U32)(triggerTimeout_sec / 10.e-6 + 0.5);
    U32 triggerTimeout_clocks = UseAutoTrigger ? 1U : 0U; // usec

    retCode =
        AlazarSetTriggerTimeOut(
            boardHandle,			// HANDLE -- board handle
            triggerTimeout_clocks	// U32 -- timeout_sec / 10.e-6 (0 means wait forever)
            );
    if (retCode != ApiSuccess)
    {
        dumpError(retCode, "[AlazarDAQ] Error: AlazarSetTriggerTimeOut failed :");
        return false;
    }

	// Configure AUX I/O connector as required
    retCode =
        AlazarConfigureAuxIO(
            boardHandle,			// HANDLE -- board handle
            AUX_OUT_TRIGGER,		// U32 -- mode
            0						// U32 -- parameter
            );
    if (retCode != ApiSuccess)
    {
        dumpError(retCode, "[AlazarDAQ] Error: AlazarConfigureAuxIO failed :");
        return false;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // FFT configuration ////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////
    if (UseFFTModule)
    {
        U32 numModules;

        // Get a handle to the FFT module
        retCode = AlazarDSPGetModules(boardHandle, 0, NULL, &numModules);
        if (numModules < 1) {
            dumpError(retCode, "[AlazarDAQ] Error: This board does any DSP modules.");
            return false;
        }

        retCode = AlazarDSPGetModules(boardHandle, numModules, &fftHandle, NULL);
        if (retCode != ApiSuccess)
        {
            dumpError(retCode, "[AlazarDAQ] Error: AlazarDSPGetModules failed :");
            return false;
        }

        // FFT Configuration
        U32 dspModuleId;
        retCode = AlazarDSPGetInfo(fftHandle, &dspModuleId, NULL, NULL, NULL, NULL, NULL);
        if (dspModuleId != DSP_MODULE_FFT)
        {
            dumpError(retCode, "[AlazarDAQ] Error: AlazarDSPGetModules failed :");
            return false;
        }

        // Create and fill the window function
        window = np::Array<float>(nScansFFT);
        retCode = AlazarDSPGenerateWindowFunction(DSP_WINDOW_HANNING, &window[0],
                                                  nScans, nScansFFT - nScans);
        if (retCode != ApiSuccess)
        {
            dumpError(retCode, "[AlazarDAQ] Error: AlazarDSPGenerateWindowFunction failed :");
            return false;
        }

        // Pad the dispersion compensation windows
        dispReal = np::Array<float>(nScansFFT);
        dispImag = np::Array<float>(nScansFFT);
        memset(dispReal, 0, sizeof(float) * nScansFFT);
        memset(dispImag, 0, sizeof(float) * nScansFFT);
        for (size_t i = 0; i < (size_t)nScans; i++)
        {
            dispReal[i] = 1.0f;
            dispImag[i] = 0.0f;
        }

        // Compute the dot product with the standard window function
        for (size_t i = 0; i < (size_t)nScansFFT; i++)
        {
            dispReal[i] *= window[i];
            dispImag[i] *= window[i];
        }

        // Set the window function
        retCode = AlazarFFTSetWindowFunction(fftHandle,
                                             nScansFFT,
                                             &dispReal[0],
                                             &dispImag[0]);
        if (retCode != ApiSuccess)
        {
            dumpError(retCode, "[AlazarDAQ] Error: AlazarFFTSetWindowFunction failed :");
            return false;
        }

        // Background subtraction
		SetBackground();
        retCode = AlazarFFTBackgroundSubtractionSetRecordS16(fftHandle, &background[0], nScans);
        if (retCode != ApiSuccess)
        {
            dumpError(retCode, "[AlazarDAQ] Error: AlazarFFTBackgroundSubtractionSetRecordS16 failed :");
            return false;
        }

        retCode = AlazarFFTBackgroundSubtractionSetEnabled(fftHandle, true);
        if (retCode != ApiSuccess)
        {
            dumpError(retCode, "[AlazarDAQ] Error: AlazarFFTBackgroundSubtractionSetEnabled failed :");
            return false;
        }
    }

    // Success to initialization
	char msg[256];
	sprintf_s(msg, 256, "[AlazarDAQ] Alazar device (system ID: %d) is successfully initialized.", SystemId);
	SendStatusMessage(msg, false);

	return true;
}

bool AlazarDAQ::set_init()
{
    if (_dirty)
    {
		if (!initialize())
		{
			char msg[256];
			sprintf_s(msg, 256, "[AlazarDAQ] Alazar device (system ID: %d) is failed to initialize.", SystemId);
			SendStatusMessage(msg, true);
			return false;
		}

        _dirty = false;
    }

    return true;
}

void AlazarDAQ::abort_acq()
{
    RETURN_CODE retCode = ApiSuccess;
    BOOL success = true;

    // Abort the acquisition
    if (!UseFFTModule)
        retCode = AlazarAbortAsyncRead(boardHandle);
    else
        retCode = AlazarDSPAbortCapture(boardHandle);

    if (retCode != ApiSuccess)
    {
        dumpError(retCode, "[AlazarDAQ] ERROR: AlazarAbortAsyncRead failed: ");
        success = FALSE;
    }

    // Free all memory allocated
    for (U32 bufferIndex = 0; bufferIndex < BUFFER_COUNT; bufferIndex++)
    {
        if (BufferArray[bufferIndex] != NULL)
            AlazarFreeBufferU16(boardHandle, BufferArray[bufferIndex]);
    }
}

void AlazarDAQ::get_background()
{
	if (UseFFTModule)
	{
		background = np::Array<int16_t>(nScans);		

		RETURN_CODE retCode = AlazarFFTBackgroundSubtractionGetRecordS16(fftHandle, background.raw_ptr(), nScans);
		if (retCode != ApiSuccess)
			dumpError(retCode, "[AlazarDAQ] ERROR: AlazarFFTBackgroundSubtractionGetRecordS16 failed: ");
	}
}

bool AlazarDAQ::startAcquisition()
{
    if (_thread.joinable())
    {
        dumpErrorSystem(::GetLastError(), "[AlazarDAQ] ERROR: Acquisition is already running: ");
        return false;
    }

    _thread = std::thread(&AlazarDAQ::run, this); // thread executing
    if (::SetThreadPriority(_thread.native_handle(), THREAD_PRIORITY_TIME_CRITICAL) == 0)
    {
        dumpErrorSystem(::GetLastError(), "[AlazarDAQ] ERROR: Failed to set acquisition thread priority: ");
        return false;
    }

	SendStatusMessage("Data acquisition thread is started.", false);

    return true;
}

void AlazarDAQ::stopAcquisition()
{
    if (_thread.joinable())
    {
        DidStopData();
        _thread.join();
    }

	SendStatusMessage("Data acquisition thread is finished normally.", false);
}


// Acquisition Thread
void AlazarDAQ::run()
{
    RETURN_CODE retCode = ApiSuccess;

    // Select the number of pre-trigger samples per record
    U32 preTriggerSamples = TriggerDelay;

    // Select the number of post-trigger samples per record
    U32 postTriggerSamples = nScans - preTriggerSamples;

    // Specify the number of records per DMA buffer
    U32 recordsPerBuffer = nAlines;

    // MEMO: we always acquire two channel and if nChannels == 1, interlace and send only 1 channel

    // Calculate the number of enabled channels from the channel mask
    int channelCount = nChannels;

    // Select which channels to capture (A, B, or both)
    U32 channelMask = (channelCount == 2) ? CHANNEL_A | CHANNEL_B : CHANNEL_A;

    // Get the sample size in bits, and the on-board memory size in samples per channel
    U8 bitsPerSample;
    U32 maxSamplesPerChannel;
    retCode = AlazarGetChannelInfo(boardHandle, &maxSamplesPerChannel, &bitsPerSample);
    if (retCode != ApiSuccess)
    {
        dumpError(retCode, "[AlazarDAQ] ERROR: AlazarGetChannelInfo failed: ");
        return;
    }

    // Calculate the size of each DMA buffer in bytes
    U32 bytesPerSample = (bitsPerSample + 7) / 8;  // sizeof(UINT16)
    U32 samplesPerRecord = preTriggerSamples + postTriggerSamples; // nScans
    U32 bytesPerRecord = bytesPerSample * samplesPerRecord; // size_of(UINT16) * nScans
    U32 bytesPerBuffer = bytesPerRecord * recordsPerBuffer * channelCount; // 2 * sizeof(UINT16) * nScans
	U32 fftSamplesPerRecord = nScansFFT;

    BOOL success = TRUE;

    // Configure the FFT
    U32 bytesPerOutputRecord = 0;
    if (UseFFTModule)
    {
        if (success)
        {
            retCode = AlazarFFTSetup(fftHandle,
                                     channelMask,
                                     samplesPerRecord,
                                     fftSamplesPerRecord,
									 FFT_OUTPUT_FORMAT_FLOAT_LOG,
                                     FFT_FOOTER_NONE,
                                     0,
                                     &bytesPerOutputRecord);
            if (retCode != ApiSuccess)
            {
                dumpError(retCode, "[AlazarDAQ] ERROR: AlazarFFTSetup failed: ");
                success = FALSE;
            }

            samplesPerRecord = bytesPerOutputRecord;
            bytesPerBuffer = bytesPerOutputRecord * recordsPerBuffer * channelCount;
        }
    }

    // Allocate memory for DMA buffers
    U32 bufferIndex;
    for (bufferIndex = 0; (bufferIndex < BUFFER_COUNT) && success; bufferIndex++)
    {
        BufferArray[bufferIndex] = (U16 *)AlazarAllocBufferU16(boardHandle, bytesPerBuffer);
        if (BufferArray[bufferIndex] == NULL)
        {
            dumpError(retCode, "[AlazarDAQ] ERROR: Alloc failed");
            success = FALSE;
        }
    }

    // Configure the record size
	if (!UseFFTModule)
	{
		if (success)
		{
			retCode =
				AlazarSetRecordSize(
					boardHandle,			// HANDLE -- board handle
					preTriggerSamples,		// U32 -- pre-trigger samples
					postTriggerSamples		// U32 -- post-trigger samples
				);
			if (retCode != ApiSuccess)
			{
				dumpError(retCode, "[AlazarDAQ] ERROR: AlazarSetRecordSize failed: ");
				success = FALSE;
			}
		}
	}

    // Configure the board to make a traditional AutoDMA acquisition
    if (success)
    {
        U32 admaFlags;

        if (UseAutoTrigger)
        {
            // Continuous mode (for debugging)
            admaFlags = ADMA_EXTERNAL_STARTCAPTURE | // Start acquisition when we call AlazarStartCapture
                ADMA_CONTINUOUS_MODE |		 // Acquire a continuous stream of sample data without trigger
                ADMA_FIFO_ONLY_STREAMING;	 // The ATS9360-FIFO does not have on-board memory
        }
        else if (UseFFTModule)
        {
            // NPT AutoMDA acquisition
            admaFlags = ADMA_EXTERNAL_STARTCAPTURE | ADMA_TRADITIONAL_MODE |
                    ADMA_NPT | ADMA_DSP;
        }
        else
		{
            // Acquire records per each trigger
            admaFlags = ADMA_EXTERNAL_STARTCAPTURE |	// Start acquisition when AlazarStartCapture is called
                ADMA_FIFO_ONLY_STREAMING |		// The ATS9360-FIFO does not have on-board memory
                ADMA_TRADITIONAL_MODE;			// Acquire multiple records optionally with pretrigger
        }

        // samples and record headers
        retCode =
            AlazarBeforeAsyncRead(
                boardHandle,                // HANDLE -- board handle
                channelMask,                // U32 -- enabled channel mask
                0,							// -(long)preTriggerSamples,	// long -- offset from trigger in samples
                samplesPerRecord,           // U32 -- samples per record
                recordsPerBuffer,           // U32 -- records per buffer
                0x7fffffff,					// U32 -- records per acquisition (infinitly)
                admaFlags                   // U32 -- AutoDMA flags
                );
        if (retCode != ApiSuccess)
        {
            dumpError(retCode, "[AlazarDAQ] ERROR: AlazarBeforeAsyncRead failed: ");
            success = FALSE;
        }
    }

    // Add the buffers to a list of buffers available to be filled by the board
    for (bufferIndex = 0; (bufferIndex < BUFFER_COUNT) && success; bufferIndex++)
    {
        U16* pBuffer = BufferArray[bufferIndex];
        retCode = AlazarPostAsyncBuffer(boardHandle, pBuffer, bytesPerBuffer);
        if (retCode != ApiSuccess)
        {
            dumpError(retCode, "[AlazarDAQ] ERROR: AlazarPostAsyncBuffer failed 1: ");
            success = FALSE;
        }
    }

    // Arm the board system to wait for a trigger event to begin the acquisition
    if (success)
    {
        retCode = AlazarStartCapture(boardHandle);
        if (retCode != ApiSuccess)
        {
            dumpError(retCode, "[AlazarDAQ] ERROR: AlazarStartCapture failed: ");
            success = FALSE;
        }
    }
	
    // Wait for each buffer to be filled, process the buffer, and re-post it to the board.
    if (success)
    {
        U32 buffersCompleted = 0, buffersCompletedUpdate = 0;
        UINT64 bytesTransferred = 0, bytesTransferredPerUpdate = 0;
        ULONG dwTickStart = 0, dwTickLastUpdate;

        // Set a buffer timeout that is longer than the time
        // required to capture all the records in one buffer.
        DWORD timeout_ms = 100;

        _running = true;
        while (_running)
        {
            // Wait for the buffer at the head of the list of available buffers
            // to be filled by the board.
            bufferIndex = buffersCompleted % BUFFER_COUNT;
            U16* pBuffer = BufferArray[bufferIndex];
			
            while (true)
            {
                if (!UseFFTModule)
                    retCode = AlazarWaitAsyncBufferComplete(boardHandle, pBuffer, timeout_ms);
                else
                    retCode = AlazarDSPGetBuffer(boardHandle, pBuffer, timeout_ms);

                if (retCode == ApiSuccess)
                    break;
                else if (retCode == ApiWaitTimeout)
                {
                    printf(".");
                    SwitchToThread();
                }
                else
                {
                    if (!UseFFTModule)
                        dumpError(retCode, "[AlazarDAQ] ERROR: AlazarWaitAsyncBufferComplete failed: ");
                    else
                        dumpError(retCode, "[AlazarDAQ] ERROR: AlazarDSPGetBuffer failed: ");
                    return;
                }

                if (!_running)
                {
                    success = FALSE;
                    break;
                }
            }
			
            if (success)
            {
                // The buffer is full and has been removed from the list
                // of buffers available for the board        
                buffersCompletedUpdate++;
                bytesTransferred += bytesPerBuffer;
                bytesTransferredPerUpdate += bytesPerBuffer;

                // Process sample data in this buffer.

                // NOTE:
                //
                // While you are processing this buffer, the board is already
                // filling the next available buffer(s).
                //
                // You MUST finish processing this buffer and post it back to the
                // board before the board fills all of its available DMA buffers
                // and on-board memory.
                //
                // Records are arranged in the buffer as follows:
                // R0[AB], R1[AB], R2[AB] ... Rn[AB]
                //
                // Samples values are arranged contiguously in each record.
                // A 12-bit sample code is stored in the most significant
                // bits of each 16-bit sample value.
                //
                // Sample codes are unsigned by default. As a result:
                // - a sample code of 0x000 represents a negative full scale input signal.
                // - a sample code of 0x800 represents a ~0V signal.
                // - a sample code of 0xFFF represents a positive full scale input signal.

                //if (nChannels == 2)
                {
					if (!UseFFTModule)
					{
						// Callback                    
						np::Array<uint16_t, 2> frame(pBuffer, channelCount * nScans, nAlines);

						// MEMO: -1 to make buffersCompleted start with 0 (same as signatec system)                    
						DidAcquireData(buffersCompleted++, frame);
					}
					else
					{
						// Callback                    
						np::Array<float, 2> frame((float*)pBuffer, channelCount * nScansFFT / 2, nAlines);

						// MEMO: -1 to make buffersCompleted start with 0 (same as signatec system)                    
						DidAcquireData(buffersCompleted++, frame);
					}
                }
//                else if (nChannels == 1)
//                {
//                    // deinterlace data and send only 1 channel

//                    // lazy initialize buffer
//                    if (buffer1ch.size(0) != nScans || buffer1ch.size(1) != nAlines)
//                    {
//                        buffer1ch = np::Array<uint16_t, 2>(nScans, nAlines);
//                        buffer2ch = np::Array<uint16_t, 2>(nScans, nAlines);
//                    }

//                    // deinterlace fringe
//                    Ipp16u *deinterlaced_fringe[2] = { buffer1ch, buffer2ch};
//                    ippsDeinterleave_16s((Ipp16s *)pBuffer, 2, nAlines * nScans, (Ipp16s **)deinterlaced_fringe);

//                    // MEMO: -1 to make buffersCompleted start with 0 (same as signatec system)
//                    DidAcquireData(buffersCompleted - 1, buffer1ch);
//                }
            }
				
            // Add the buffer to the end of the list of available buffers.
            if (success)
            {
                retCode = AlazarPostAsyncBuffer(boardHandle, pBuffer, bytesPerBuffer);
                if (retCode != ApiSuccess)
                {
                    //dumpError(retCode, "[AlazarDAQ] ERROR: AlazarPostAsyncBuffer failed 2: ");
                    //success = FALSE;
                }
            }

            // If the acquisition failed, exit the acquisition loop
            if (!success)
                break;

            // Acquisition Status
            if (!dwTickStart)
                dwTickStart = dwTickLastUpdate = GetTickCount();

            // Periodically update progress
            ULONG dwTickNow = GetTickCount();
            if (dwTickNow - dwTickLastUpdate > 5000)
            {
                double dRate, dRateUpdate;

                ULONG dwElapsed = dwTickNow - dwTickStart;
                ULONG dwElapsedUpdate = dwTickNow - dwTickLastUpdate;

                dwTickLastUpdate = dwTickNow;

                if (dwElapsed)
                {
                    dRate = (bytesTransferred / 1000000.0) / (dwElapsed / 1000.0);
                    dRateUpdate = (bytesTransferredPerUpdate / 1000000.0) / (dwElapsedUpdate / 1000.0);
					frameRate = (double)buffersCompletedUpdate / (double)(dwElapsedUpdate) * 1000.0;

                    unsigned h = 0, m = 0, s = 0;
                    if (dwElapsed >= 1000)
                    {
                        if ((s = dwElapsed / 1000) >= 60)	// Seconds
                        {
                            if ((m = s / 60) >= 60)			// Minutes
                            {
                                if (h = m / 60)				// Hours
                                    m %= 60;
                            }
                            s %= 60;
                        }
                    }

					char msg[MAX_MSG_LENGTH];
					sprintf(msg, "[AlazarDAQ] [SystemId: %d] [Elapsed Time] %u:%02u:%02u [DAQ Rate] %4.2f MB/s [Frame Rate] %3.2f fps ", 
						SystemId, h, m, s, dRateUpdate, frameRate);
					SendStatusMessage(msg, false);
                }

                // reset
                buffersCompletedUpdate = 0;
                bytesTransferredPerUpdate = 0;
            }
        }
    }

    // Abort the acquisition
    abort_acq();
}

// Dump a Alazar library error
void AlazarDAQ::dumpError(RETURN_CODE retCode, const char* pPreamble)
{
	char *pErr = nullptr;
	int my_res;
	char msg[MAX_MSG_LENGTH];
	memcpy(msg, pPreamble, strlen(pPreamble) + 1);

    strcat_s(msg, MAX_MSG_LENGTH, AlazarErrorToText(retCode));

    SendStatusMessage(msg, true);

    // Abort the acquisition
    abort_acq();
}


void AlazarDAQ::dumpErrorSystem(int res, const char* pPreamble)
{
    char* pErr = nullptr;
    char msg[MAX_MSG_LENGTH];
    memcpy(msg, pPreamble, strlen(pPreamble));

    sprintf(pErr, "Error code (%d)", res);
    strcat_s(msg, MAX_MSG_LENGTH, pErr);

    SendStatusMessage(msg, true);

    // Abort the acquisition
    abort_acq();
}
