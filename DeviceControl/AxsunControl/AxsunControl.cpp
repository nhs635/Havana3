
#include "AxsunControl.h"


AxsunControl::AxsunControl() :
	m_pAxsunOCTControl(nullptr),
	m_bIsConnected(DISCONNECTED),
	m_daq_device(-1),
	m_laser_device(-1)
{
}


AxsunControl::~AxsunControl()
{
	if (m_bIsConnected == CONNECTED)
	{
		unsigned long retvallong;
		m_pAxsunOCTControl->CloseConnections();
		m_pAxsunOCTControl->StopNetworkControlInterface(&retvallong);
        CoUninitialize();
		SendStatusMessage("[Axsun Control] Control Connections is successfully closed and uninitialized.", false);
	}
}


bool AxsunControl::initialize(int n_device)
{
	HRESULT result;
	unsigned long retvallong;
	const char* pPreamble = "[Axsun Control] Failed to initialize Axsun OCT control devices: ";

	SendStatusMessage("[Axsun Control] Initializing control devices...", false);

	// Co-Initialization
    result = CoInitialize(NULL);
	
	// Dynamic Object for Axsun OCT Control
	m_pAxsunOCTControl = IAxsunOCTControlPtr(__uuidof(struct AxsunOCTControl));
	
	// Start Ethernet Connection
	result = m_pAxsunOCTControl->StartNetworkControlInterface(&retvallong);
	if (result != S_OK)
	{
		dumpControlError(result, pPreamble);
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// Connect Device (DAQ and/or Laser)
	unsigned long deviceNum = 0;
	BSTR systemTypeString = SysAllocString(L"");
	for (int i = 0; i < n_device; i++) // DAQ [42], Light Source [40]
	{
		result = m_pAxsunOCTControl->ConnectToOCTDevice(i, &m_bIsConnected);
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}

		if (m_bIsConnected == CONNECTED)
		{
			result = m_pAxsunOCTControl->GetSystemType(&deviceNum, &systemTypeString, &retvallong);
			if (result != S_OK)
			{
				dumpControlError(result, pPreamble);
				return false;
			}
			
			if (deviceNum == DAQ_DEVICE)
				m_daq_device = i;
			else if (deviceNum == LASER_DEVICE)
				m_laser_device = i;

			char msg[256];
			sprintf(msg, "[Axsun Control] %S (deviceNum: %d) is successfully connected.", systemTypeString, deviceNum);
			SendStatusMessage(msg, false);
		}
		else
		{
			result = 80;
			dumpControlError(result, pPreamble);
			SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
			return false;
		}
	}
	SysFreeString(systemTypeString);

	if (m_daq_device > 0)
	{
		// Image_Sync Setting (LVCMOS)
		if (writeFPGARegSingleBit(2, 11, false) != true) return false; // external image sync
		if (writeFPGARegSingleBit(2, 9, true) != true) return false;   // LVCMOS input (X)
		if (writeFPGARegSingleBit(2, 10, true) != true) return false;
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}
		SendStatusMessage("[Axsun Control] Image_Sync mode is set to LVCMOS.", false);

		// Image Channel Setting (Ch 1 H Only)
		if (writeFPGARegSingleBit(20, 13, true) != true) return false;
		if (writeFPGARegSingleBit(20, 5, true) != true) return false;
		SendStatusMessage("[Axsun Control] Ch 1(H) signal is considered only.", false);
	}

	return true;
}


bool AxsunControl::setLaserEmission(bool status)
{
	HRESULT result;
	unsigned long retvallong;
	const char* pPreamble = "[Axsun Control] Failed to set Laser Emission: ";
	
	result = m_pAxsunOCTControl->ConnectToOCTDevice(m_laser_device, &m_bIsConnected);
	if (result != S_OK)
	{
		dumpControlError(result, pPreamble);
		return false;
	}

	if (m_bIsConnected == CONNECTED)
	{
		if (status)
			result = m_pAxsunOCTControl->StartScan(&retvallong);
		else
			result = m_pAxsunOCTControl->StopScan(&retvallong);
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}
	}
	else
	{
		result = 80;
		dumpControlError(result, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}

	char msg[256];
	sprintf(msg, "[Axsun Control] Laser Emission is turned %s.", status ? "on" : "off");
	SendStatusMessage(msg, false);

	return true;
}

bool AxsunControl::setLiveImagingMode(bool status)
{
	HRESULT result;
	const char* pPreamble = "[Axsun Control] Failed to set Live Imaging mode: ";

	result = m_pAxsunOCTControl->ConnectToOCTDevice(m_daq_device, &m_bIsConnected);
	if (result != S_OK)
	{
		dumpControlError(result, pPreamble);
		return false;
	}

	if (m_bIsConnected == CONNECTED)
	{
		if (status)
		{
			if (writeFPGARegSingleBit(19, 15, true) != true) return false;
			if (writeFPGARegSingleBit(2, 2, true) != true) return false;
            //if (writeFPGARegSingleBit(31, 0, false) != true) return false; // burst recording
		}
		else
		{
			if (writeFPGARegSingleBit(2, 2, false) != true) return false;
			if (writeFPGARegSingleBit(19, 15, false) != true) return false;
		}
	}
	else
	{
		result = 80;
		dumpControlError(result, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}
	
	char msg[256];
    sprintf(msg, "[Axsun Control] Live Imaging is %s.", status ? "started" : "stopped");
	SendStatusMessage(msg, false);

	return true;
}


//bool AxsunOCT::setBurstRecording(unsigned short n_images, bool recording)
//{
//    HRESULT result;
//    unsigned long retvallong;
//    const char* pPreamble = "[Axsun Control] Failed to set burst recording: ";
//
//    result = m_pAxsunOCTControl->ConnectToOCTDevice(DAQ_DEVICE, &m_bIsConnected);
//    if (result != S_OK)
//    {
//        dumpControlError(result, pPreamble);
//        return false;
//    }
//
//    if (m_bIsConnected == CONNECTED)
//    {
//        if (recording)
//        {
//            result = m_pAxsunOCTControl->SetFPGARegister(33, n_images, &retvallong);
//            if (result != S_OK)
//            {
//                dumpControlError(result, pPreamble);
//                return false;
//            }
//        }
//        if (writeFPGARegSingleBit(31, 0, recording) != true) return false;
//
//        if (recording)
//            printf("[Axsun Control] Burst recording is started (n_images: %d).", n_images);
//        else
//            printf("[Axsun Control] Burst recording is finished.");
//    }
//    else
//    {
//        result = 80;
//        dumpControlError(result, pPreamble);
//        printf("[Axsun Control] Unable to connect to the devices.");
//        return false;
//    }
//
//    return true;
//}
//
//

bool AxsunControl::setBackground(const unsigned short* pBackground)
{
    HRESULT result = 0;
	unsigned long retvallong;
	const char* pPreamble = "[Axsun Control] Failed to set background: ";

	result = m_pAxsunOCTControl->ConnectToOCTDevice(m_daq_device, &m_bIsConnected);
	if (result != S_OK)
	{
		dumpControlError(result, pPreamble);
		return false;
	}

	if (m_bIsConnected == CONNECTED)
	{
		// Send array to FPGA register
		for (int i = 0; i < ALINE_LENGTH; i++)
		{
			result = m_pAxsunOCTControl->SetFPGARegister(37, (pBackground != nullptr) ? pBackground[i] : 0x0000, &retvallong);
			if (result != S_OK)
			{
				dumpControlError(result, pPreamble);
				return false;
			}
			DidTransferArray(i);
		}

		if (pBackground)
			SendStatusMessage("[Axsun Control] Background is successfully set.", false);
		else
			SendStatusMessage("[Axsun Control] Background is set to zero.", false);
	}
	else
	{
		result = 80;
		dumpControlError(result, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}

	return true;
}


bool AxsunControl::setDispersionCompensation(double a2, double a3, int length)
{
    HRESULT result;
    unsigned long retvallong;
    const char* pPreamble = "[Axsun Control] Failed to set dispersion compensating window function: ";

    result = m_pAxsunOCTControl->ConnectToOCTDevice(m_daq_device, &m_bIsConnected);
    if (result != S_OK)
    {
        dumpControlError(result, pPreamble);
        return false;
    }

    if (m_bIsConnected == CONNECTED)
    {
        if (length <= MAX_SAMPLE_LENGTH)
        {
            short* win_real = new short[MAX_SAMPLE_LENGTH]; memset(win_real, 0, sizeof(short) * MAX_SAMPLE_LENGTH);
            short* win_imag = new short[MAX_SAMPLE_LENGTH]; memset(win_imag, 0, sizeof(short) * MAX_SAMPLE_LENGTH);

			double temp_win = 0;
            double pi = 3.14159265358979323846;
            for (int i = 0; i < length; i++)
            {
                temp_win = 32767.0 * (1 - cos(2.0 * pi * i / (length - 1))) / 2.0;
                win_real[i] = short(temp_win * cos(a2 * 1e-6 * pow(i - length / 2, 2) + a3 * 1e-9 * pow(i - length / 2, 3)));
                win_imag[i] = short(temp_win * sin(a2 * 1e-6 * pow(i - length / 2, 2) + a3 * 1e-9 * pow(i - length / 2, 3)));
            }
			
            // Real Part of Dispersion Compensating Window
            if (writeFPGARegSingleBit(20, 14, false) != true) return false;
			for (int i = 0; i < MAX_SAMPLE_LENGTH; i++)
			{
				result = m_pAxsunOCTControl->SetFPGARegister(25, win_real[i], &retvallong);
				if (result != S_OK)
				{
					dumpControlError(result, pPreamble);
					return false;
				}
				DidTransferArray(i);
			}

            // Imag Part of Dispersion Compensating Window
            if (writeFPGARegSingleBit(20, 14, true) != true) return false;
			for (int i = 0; i < MAX_SAMPLE_LENGTH; i++)
			{
				result = m_pAxsunOCTControl->SetFPGARegister(25, win_imag[i], &retvallong);
				if (result != S_OK)
				{
					dumpControlError(result, pPreamble);
					return false;
				}
				DidTransferArray(MAX_SAMPLE_LENGTH + i);
			}
			
            delete[] win_real;
            delete[] win_imag;

			char msg[256];
            sprintf(msg, "[Axsun Control] Dispcomp window is successfully defined (a2 = %.1f, a3 = %.1f, len = %d).", a2, a3, length);
			SendStatusMessage(msg, false);
        }
        else
        {
            result = 82;
            dumpControlError(result, pPreamble);
			SendStatusMessage("[Axsun Control] Too long sample length.", false);
            return false;
        }
    }
    else
    {
        result = 80;
        dumpControlError(result, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
        return false;
    }

    return true;
}


bool AxsunControl::setBypassMode(bypass_mode _bypass_mode)
{
    HRESULT result;
    unsigned long retvallong;
    const char* pPreamble = "[Axsun Control] Failed to set bypass mode: ";

    result = m_pAxsunOCTControl->ConnectToOCTDevice(m_daq_device, &m_bIsConnected);
    if (result != S_OK)
    {
        dumpControlError(result, pPreamble);
        return false;
    }

    if (m_bIsConnected == CONNECTED)
    {
		unsigned int M = 1;
		unsigned short bypass_config = 0;
		const char* mode_name;

		switch (_bypass_mode)
		{
		//case raw_adc_data:
		//	M = 8;
		//	bypass_config = 0x81FC;
		//	mode_name = "Raw ADC Data";
		//	break;
		//case windowed_data:
		//	M = 16;
		//	bypass_config = 0x80FE;
		//	dataType = cmplx;
		//	mode_name = "Windowed Data";
		//	break;
		//case post_fft:
		//	M = 8;
		//	bypass_config = 0x007E;
		//	dataType = cmplx;
		//	mode_name = "Post-FFT";
		//	break;
		//case abs2:
		//	M = 8;
		//	bypass_config = 0x003E;
		//	dataType = u32;
		//	mode_name = "Abs()2";
		//	break;
		case abs2_with_bg_subtracted:
			M = 10;
			bypass_config = 0x001C;
			mode_name = "Abs() with BG Subtracted";
			break;
		//case log_compressed:
		//	M = 4;
		//	bypass_config = 0x000C;
		//	mode_name = "Log Compressed";
		//	break;
		//case dynamic_range_reduced:
		//	M = 4;
		//	bypass_config = 0x0002;
		//	mode_name = "Dynamic Range Reduced";
		//	break;
		case jpeg_compressed:
			M = 1;
			bypass_config = 0x0000;
			mode_name = "JPEG Compressed";
			break;
		default:
			result = 81;
			dumpControlError(result, pPreamble);
			SendStatusMessage("[Axsun Control] Unsupported bypass mode.", false);
			return false;
		}

		result = m_pAxsunOCTControl->SetFPGARegister(60, M - 1, &retvallong);
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}
		result = m_pAxsunOCTControl->SetFPGARegister(61, bypass_config, &retvallong);
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}
		result = m_pAxsunOCTControl->SetFPGARegister(61, bypass_config + 1, &retvallong);
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}

		char msg[256];
		sprintf(msg, "[Axsun Control] Bypass mode is set to %s (subsampling factor: %d).", mode_name, M);
		SendStatusMessage(msg, false);
    }
    else
    {
        result = 80;
        dumpControlError(result, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
        return false;
    }

    return true;
}


bool AxsunControl::setVDLHome()
{
	std::unique_lock<std::mutex> lock(vdl_mutex);
	{
		HRESULT result;
		unsigned long retvallong;
		const char* pPreamble = "[Axsun Control] Failed to set VDL home: ";

		result = m_pAxsunOCTControl->ConnectToOCTDevice(m_laser_device, &m_bIsConnected);
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}

		if (m_bIsConnected == CONNECTED)
		{
			result = m_pAxsunOCTControl->HomeVDL(&retvallong);
			if (result != S_OK)
			{
				dumpControlError(result, pPreamble);
				return false;
			}
			SendStatusMessage("[Axsun Control] VDL length is set to home.", false);
		}
		else
		{
			result = 80;
			dumpControlError(result, pPreamble);
			SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
			return false;
		}
	}

	return true;
}


bool AxsunControl::setVDLLength(float position)
{
	std::unique_lock<std::mutex> lock(vdl_mutex);
	{
		HRESULT result;
		unsigned long retvallong;
		const char* pPreamble = "[Axsun Control] Failed to set VDL length: ";

		result = m_pAxsunOCTControl->ConnectToOCTDevice(m_laser_device, &m_bIsConnected);
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}

		if (m_bIsConnected == CONNECTED)
		{
			result = m_pAxsunOCTControl->MoveAbsVDL(position, 5, &retvallong);
			if (result != S_OK)
			{
				dumpControlError(result, pPreamble);
				return false;
			}

			char msg[256];
			sprintf(msg, "[Axsun Control] VDL length is set to %.2f mm.", position);
			SendStatusMessage(msg, false);
		}
		else
		{
			result = 80;
			dumpControlError(result, pPreamble);
			SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
			return false;
		}
	}

	return true;
}


bool AxsunControl::setClockDelay(unsigned long delay)
{
	HRESULT result;
	unsigned long retvallong;
	const char* pPreamble = "[Axsun Control] Failed to set clock delay: ";

    result = m_pAxsunOCTControl->ConnectToOCTDevice(m_laser_device, &m_bIsConnected);
	if (result != S_OK)
	{
		dumpControlError(result, pPreamble);
		return false;
	}

	if (m_bIsConnected == CONNECTED)
    {
        unsigned long delay0;
        result = m_pAxsunOCTControl->SetClockDelay(delay, &retvallong);
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}
        result = m_pAxsunOCTControl->GetClockDelay(&delay0, &retvallong);

		char msg[256];
        sprintf(msg, "[Axsun Control] Clock delay is set to %.3f nsec [%d].", CLOCK_GAIN * (double)delay + CLOCK_OFFSET, delay0);
		SendStatusMessage(msg, false);
	}
	else
	{
		result = 80;
		dumpControlError(result, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}

	return true;
}


bool AxsunControl::setDecibelRange(double min_dB, double max_dB)
{
    double gain = 1275.0 * log10(2) / (max_dB - min_dB);
    double offset = -gain * (min_dB / 10.0 / log10(2) + 6.0);

    if ((offset > 127.996) || (offset < -128.000) || (gain < 0) || (gain > 15.9997))
    {
		char msg[256];
        sprintf(msg, "[Axsun Control] Invalid decibel range... (offset: %.3f / gain: %.4f)", offset, gain);
		SendStatusMessage(msg, false);

        return false;
    }

    if (setOffset(offset) != true) return false;
    if (setGain(gain) != true) return false;

	char msg[256];
    sprintf(msg, "[Axsun Control] Decibel range is set to [%.1f %.1f] (offset: %.3f / gain: %.4f)", min_dB, max_dB, offset, gain);
	SendStatusMessage(msg, false);


    return true;
}

bool AxsunControl::getDeviceState()
{
	HRESULT result;
	//unsigned long retvallong;
	const char* pPreamble = "[Axsun Control] Failed to get device state: ";

	result = m_pAxsunOCTControl->ConnectToOCTDevice(m_laser_device, &m_bIsConnected);
	if (result != S_OK)
	{
		dumpControlError(result, pPreamble);
		return false;
	}

	if (m_bIsConnected == CONNECTED)
	{
		//unsigned long laser_time;
		//result = m_pAxsunOCTControl->GetLaserOnTime(&laser_time, &retvallong);
		//if (result != S_OK)
		//{
		//	dumpControlError(result, pPreamble);
		//	return false;
		//}

		//unsigned long rawVal;
		//float scaledVal;
		//BSTR temp; temp = _bstr_t("test");
		////result = m_pAxsunOCTControl->Get GetLowSpeedADOneChannel(5, &rawVal, &scaledVal, &temp);
		//if (result != S_OK)
		//{
		//	dumpControlError(result, pPreamble);
		//	return false;
		//}		
		//printf("[%d] %d %f %s\n", laser_time, rawVal, scaledVal, temp);

		//unsigned long delay0;
		//result = m_pAxsunOCTControl->SetClockDelay(delay, &retvallong);
		//if (result != S_OK)
		//{
		//	dumpControlError(result, pPreamble);
		//	return false;
		//}
		//result = m_pAxsunOCTControl->GetClockDelay(&delay0, &retvallong);

		//char msg[256];
		//sprintf(msg, "[Axsun Control] Clock delay is set to %.3f nsec [%d].", CLOCK_GAIN * (double)delay + CLOCK_OFFSET, delay0);
		//SendStatusMessage(msg, false);
	}
	else
	{
		result = 80;
		dumpControlError(result, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}

	return true;
}



bool AxsunControl::writeFPGARegSingleBit(unsigned long regNum, int bitNum, bool set)
{
	HRESULT result;
	unsigned long retvallong;
	const char* pPreamble = "[Axsun Control] Failed to write FPGA single register bit: ";

	result = m_pAxsunOCTControl->ConnectToOCTDevice(m_daq_device, &m_bIsConnected);
	if (result != S_OK)
	{
		dumpControlError(result, pPreamble);
		return false;
	}

	if (m_bIsConnected == CONNECTED)
	{			
		unsigned short regVal = 0;
		result = m_pAxsunOCTControl->GetFPGARegister(regNum, &regVal, &retvallong);
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}
		//printf("[%d-%d-%s] RegVal : 0x%04x", regNum, bitNum, set ? "set" : "clr", regVal);	

		unsigned short regMask = 1 << bitNum;
		if (set) 
			regVal = regVal | regMask;
		else
		{
			regMask = ~regMask;
			regVal = regVal & regMask;
		}
		//printf("[%d-%d-%s] RegMask: 0x%04x", regNum, bitNum, set ? "set" : "clr", regMask);
		//printf("[%d-%d-%s] RegVal1: 0x%04x", regNum, bitNum, set ? "set" : "clr", regVal);
		
		result = m_pAxsunOCTControl->SetFPGARegister(regNum, regVal, &retvallong);
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}
	}
	else
	{
		result = 80;
		dumpControlError(result, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}

	return true;
}


bool AxsunControl::setOffset(double offset)
{
	HRESULT result;
	unsigned long retvallong;
	const char* pPreamble = "[Axsun Control] Failed to set Offset: ";

	result = m_pAxsunOCTControl->ConnectToOCTDevice(m_daq_device, &m_bIsConnected);
	if (result != S_OK)
	{
		dumpControlError(result, pPreamble);
		return false;
	}

	if (m_bIsConnected == CONNECTED)
	{
		bool bit[17] = { 0, };

		double offset0 = offset;
		if (offset0 < 0)
		{
			bit[16] = 1;
			offset0 = 128.0 + offset0;
		}
		offset0 = floor(1000.0 * offset0) / 1000.0;

		double integer_part = floor(offset0);
		double fraction_part = offset0 - integer_part;

		for (int i = 0; i < 7; i++)
		{
			bit[i + 9] = ((char)integer_part % 2) == 1;
			integer_part = floor(integer_part / 2);
		}

		for (int i = 0; i < 9; i++)
		{ 
			fraction_part = 2 * fraction_part;
			bit[8 - i] = (int)(floor(fraction_part)) == 1;
			fraction_part = fraction_part - floor(fraction_part);
		}

		unsigned short regVal = 0;
		for (int i = 0; i < 16; i++)
			regVal += bit[i + 1] << i;
		regVal += bit[0];

		result = m_pAxsunOCTControl->SetFPGARegister(23, regVal, &retvallong);
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}

		char msg[256];
		sprintf(msg, "[Axsun Control] Offset value is set to %.3f (0x%04X).", offset, regVal);
		SendStatusMessage(msg, false);
	}	
	else
	{
		result = 80;
		dumpControlError(result, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}

	return true;
}


bool AxsunControl::setGain(double gain)
{
	HRESULT result;
	unsigned long retvallong;
	const char* pPreamble = "[Axsun Control] Failed to set Gain: ";

	result = m_pAxsunOCTControl->ConnectToOCTDevice(m_daq_device, &m_bIsConnected);
	if (result != S_OK)
	{
		dumpControlError(result, pPreamble);
		return false;
	}

	if (m_bIsConnected == CONNECTED)
	{
		bool bit[17] = { 0, };

		gain = floor(10000.0 * gain) / 10000.0;

		double integer_part = floor(gain);
		double fraction_part = gain - integer_part;

		for (int i = 0; i < 4; i++)
		{
			bit[i + 13] = ((char)integer_part % 2) == 1;
			integer_part = floor(integer_part / 2);
		}

		for (int i = 0; i < 13; i++)
		{
			fraction_part = 2 * fraction_part;
			bit[12 - i] = (int)(floor(fraction_part)) == 1;
			fraction_part = fraction_part - floor(fraction_part);
		}

		unsigned short regVal = 0;
		for (int i = 0; i < 16; i++)
			regVal += bit[i + 1] << i;
		regVal += bit[0];

		result = m_pAxsunOCTControl->SetFPGARegister(24, regVal, &retvallong);
		if (result != S_OK)
		{
			dumpControlError(result, pPreamble);
			return false;
		}

		char msg[256];
		sprintf(msg, "[Axsun Control] Gain value is set to %.4f (0x%04X).", gain, regVal);
		SendStatusMessage(msg, false);
	}
	else
	{
		result = 80;
		dumpControlError(result, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}

	return true;
}


void AxsunControl::dumpControlError(long res, const char* pPreamble)
{
    char msg[256] = { 0, };
    memcpy(msg, pPreamble, strlen(pPreamble));

    char err[256] = { 0, };
    sprintf_s(err, 256, "Control error code (%d)", res);
	
    strcat(msg, err);

	SendStatusMessage(msg, true);
}