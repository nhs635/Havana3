
#include "AxsunControl.h"

#include <ipps.h>
#include <ippi.h>


// Convert AxDevType enum into string
std::string AxDevTypeString(AxDevType device) {
	switch (device) {
	case AxDevType::LASER: return "Laser";
	case AxDevType::EDAQ: return "EDAQ";
	case AxDevType::CLDAQ: return "CLDAQ";
	default: return "Unknown";
	}
}

// Convert AxConnectionType enum into string
std::string AxConnectTypeString(AxConnectionType connection) {
	switch (connection) {
	case AxConnectionType::USB: return "USB";
	case AxConnectionType::RS232: return "RS-232";
	case AxConnectionType::RS232_PASSTHROUGH: return "via EDAQ";
	case AxConnectionType::ETHERNET: return "Ethernet";
	default: return "Unknown";
	}
}

// Convert AxPipelineMode enum into string
std::string AxPipelineModeString(AxPipelineMode mode) {
	switch (mode) {
	case AxPipelineMode::RAW_ADC: return "RAW_ADC";
	case AxPipelineMode::WINDOWED: return "RS-WINDOWED";
	case AxPipelineMode::IFFT: return "IFFT";
	case AxPipelineMode::MOD_SQUARED: return "MOD_SQUARED";
	case AxPipelineMode::SQRT: return "SQRT";
	case AxPipelineMode::LOG: return "LOG";
	case AxPipelineMode::EIGHT_BIT: return "EIGHT_BIT";
	case AxPipelineMode::JPEG_COMP: return "JPEG_COMP";
	default: return "Unknown";
	}
}

// Convert AxTECState enum into string
std::string AxTECStateString(AxTECState state) {
	switch (state) {
	case AxTECState::TEC_UNINITIALIZED: return "TEC has not been initialized.";
	case AxTECState::WARMING_UP: return "RS-TEC is stabilizing (warming up).";
	case AxTECState::WAITING_IN_RANGE: return "TEC is stabilizing (waiting).";
	case AxTECState::READY: return "TEC is ready.";
	case AxTECState::NOT_INSTALLED: return "TEC is not installed.";
	case AxTECState::ERROR_NEVER_GOT_TO_READY: return "TEC error: never got to ready state.";
	case AxTECState::ERROR_WENT_OUT_OF_RANGE: return "TEC error: temperature went out of range.";
	default: return "Unknown";
	}
}


AxsunControl::AxsunControl() :
	m_bInitialized(false),
	m_daq_device(AxDevType::UNDEFINED),
	m_laser_device(AxDevType::UNDEFINED)
{
	background_frame = np::Uint16Array2(NUM_ALINES, ALINE_LENGTH);
	background_vector = std::vector<uint16_t>(ALINE_LENGTH, 0);
}

AxsunControl::~AxsunControl()
{
	axCloseAxsunOCTControl();
	SendStatusMessage("[Axsun Control] Control Connections is successfully closed and uninitialized.", false);
}


bool AxsunControl::initialize()
{
	AxErr retval = AxErr::NO_AxERROR;
	const char* pPreamble = "[Axsun Control] Failed to initialize Axsun OCT control devices: ";
	
	SendStatusMessage("[Axsun Control] Initializing control devices...", false);

	// Open Axsun OCT control instance
	retval = axOpenAxsunOCTControl(0);
	if (retval != AxErr::NO_AxERROR)
	{
		dumpControlError(retval, pPreamble);
		return false;
	}

	// Get AxsunOCTControl_LW version
	uint32_t major, minor, patch, build;
	retval = axLibraryVersion(&major, &minor, &patch, &build);
	if (retval != AxErr::NO_AxERROR)
	{
		dumpControlError(retval, pPreamble);
		return false;
	}
	else
	{
		char msg[256];
		sprintf(msg, "[Axsun Control] Using AxsunOCTControl_LW library version: v%d.%d.%d.%d", major, minor, patch, build);
		SendStatusMessage(msg, false);
	}

	// Open Ethernet network interface (for Ethernet DAQ connectivity only)
	retval = axNetworkInterfaceOpen(1);
	if (retval != AxErr::NO_AxERROR)
	{
		dumpControlError(retval, pPreamble);
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// Connect Device (DAQ and/or Laser)	
	uint32_t count = axCountConnectedDevices();
	if (count > 0)
	{
		for (uint32_t i = 0; i < count; i++)
		{
			// Get connection type
			AxConnectionType connection_type;
			retval = axConnectionType(&connection_type, i);
			if (retval != AxErr::NO_AxERROR)
			{
				dumpControlError(retval, pPreamble);
				return false;
			}

			// Get device type
			AxDevType device_type;
			retval = axDeviceType(&device_type, i);
			if (retval != AxErr::NO_AxERROR)
			{
				dumpControlError(retval, pPreamble);
				return false;
			}

			// Get firmware version
			uint32_t major, minor, patch;
			retval = axFirmwareVersion(&major, &minor, &patch, i);
			if (retval != AxErr::NO_AxERROR)
			{
				dumpControlError(retval, pPreamble);
				return false;
			}

			// Get serial number
			char serial_no[40];
			retval = axSerialNumber(serial_no, i);

			// Allocate device number
			if (device_type == AxDevType::EDAQ)
				m_daq_device = AxDevType::EDAQ;
			else if (device_type == AxDevType::LASER)
				m_laser_device = AxDevType::LASER;

			// Print device information
			char msg[256];
			sprintf(msg, "[Axsun Control] %s (v%d.%d.%d / SN: %s) is successfully connected through %s.",
				AxDevTypeString(device_type).c_str(), major, minor, patch, serial_no,
				AxConnectTypeString(connection_type).c_str());
			SendStatusMessage(msg, false);
		}
	}
	else
	{
		dumpControlError(retval, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}
	
	if (m_daq_device == AxDevType::EDAQ)
	{
		// Image_Sync Setting (LVCMOS)
		if (writeFPGARegSingleBit(2, 11, false) != true) return false; // external image sync
		if (writeFPGARegSingleBit(2, 9, true) != true) return false;   // LVCMOS input (X)
		if (writeFPGARegSingleBit(2, 10, true) != true) return false;
		SendStatusMessage("[Axsun Control] Image_Sync mode is set to LVCMOS.", false);

		// Image Channel Setting (Ch 1 H Only)
		if (writeFPGARegSingleBit(20, 13, true) != true) return false;
		if (writeFPGARegSingleBit(20, 5, true) != true) return false;
		SendStatusMessage("[Axsun Control] Ch 1(H) signal is considered only.", false);

		// Initialization flag
		m_bInitialized = true;
	}

	return true;
}


bool AxsunControl::setLaserEmission(bool status)
{
	AxErr retval = AxErr::NO_AxERROR;
	const char* pPreamble = "[Axsun Control] Failed to set Laser Emission: ";
	
	if (m_laser_device == AxDevType::LASER)
	{
		retval = axSetLaserEmission(status, 0);
		if ((retval != AxErr::NO_AxERROR) && ((int)retval != -40))
		{
			dumpControlError(retval, pPreamble);
			return false;
		}
	}
	else
	{
		dumpControlError(retval, pPreamble);
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
	AxErr retval = AxErr::NO_AxERROR;
	const char* pPreamble = "[Axsun Control] Failed to set Live Imaging mode: ";
	
	if (m_daq_device == AxDevType::EDAQ)
	{
		if (status)
		{
			if (writeFPGARegSingleBit(19, 15, true) != true) return false;
			if (writeFPGARegSingleBit(2, 2, true) != true) return false;
            ///if (writeFPGARegSingleBit(31, 0, false) != true) return false; // burst recording
		}
		else
		{
			if (writeFPGARegSingleBit(2, 2, false) != true) return false;
			if (writeFPGARegSingleBit(19, 15, false) != true) return false;
		}
	}
	else
	{
		dumpControlError(retval, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}
	
	char msg[256];
    sprintf(msg, "[Axsun Control] Live Imaging is %s.", status ? "started" : "stopped");
	SendStatusMessage(msg, false);

	return true;
}


bool AxsunControl::setBackground()
{
	AxErr retval = AxErr::NO_AxERROR;
	const char* pPreamble = "[Axsun Control] Failed to set background: ";

	if (m_daq_device == AxDevType::EDAQ)
	{			
		for (int i = 0; i < ALINE_LENGTH; i++)
		{
			double mean;
			ippiMean_16u_C1R(&background_frame(0, i), sizeof(uint16_t) * NUM_ALINES / 2, { NUM_ALINES / 2, 1 }, &mean);
			background_vector[i] = (uint16_t)mean;
		}
		
		// Background subtraction
		retval = axSetFPGADataArray(37, background_vector.data(), background_vector.size(), 0);
		if (retval != AxErr::NO_AxERROR)
		{
			dumpControlError(retval, pPreamble);
			return false;
		}

		char msg[256];
		sprintf(msg, "[Axsun Control] background subtraction is successfully set.");;
		SendStatusMessage(msg, false);
	}
	else
	{
		dumpControlError(retval, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}

	return true;
}


bool AxsunControl::setDispersionCompensation(double a2, double a3, int length)
{
	AxErr retval = AxErr::NO_AxERROR;
    const char* pPreamble = "[Axsun Control] Failed to set dispersion compensating window function: ";
	
	if (m_daq_device == AxDevType::EDAQ)
    {
        if (length <= MAX_SAMPLE_LENGTH)
        {
			std::vector<uint16_t> win_real(MAX_SAMPLE_LENGTH, 0);
			std::vector<uint16_t> win_imag(MAX_SAMPLE_LENGTH, 0);

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
			retval = axSetFPGADataArray(25, win_real.data(), win_real.size(), 0);
			if (retval != AxErr::NO_AxERROR)
			{
				dumpControlError(retval, pPreamble);
				return false;
			}
			
			// Imag Part of Dispersion Compensating Window
            if (writeFPGARegSingleBit(20, 14, true) != true) return false;
			retval = axSetFPGADataArray(25, win_imag.data(), win_imag.size(), 0);
			if (retval != AxErr::NO_AxERROR)
			{
				dumpControlError(retval, pPreamble);
				return false;
			}
			
			char msg[256];
            sprintf(msg, "[Axsun Control] Dispcomp window is successfully defined (a2 = %.1f, a3 = %.1f, len = %d).", a2, a3, length);
			SendStatusMessage(msg, false);
        }
        else
        {
            dumpControlError(retval, pPreamble);
			SendStatusMessage("[Axsun Control] Too long sample length.", false);
            return false;
        }
    }
    else
    {
        dumpControlError(retval, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
        return false;
    }

    return true;
}


bool AxsunControl::setPipelineMode(AxPipelineMode pipeline_mode)
{
	AxErr retval = AxErr::NO_AxERROR;
    const char* pPreamble = "[Axsun Control] Failed to set bypass mode: ";

	if (m_daq_device == AxDevType::EDAQ)
    {
		uint8_t subsampling_factor = 1;
		switch (pipeline_mode) {
		case AxPipelineMode::RAW_ADC: subsampling_factor = 8;
		case AxPipelineMode::WINDOWED: subsampling_factor = 16;
		case AxPipelineMode::IFFT: subsampling_factor = 8;
		case AxPipelineMode::MOD_SQUARED:subsampling_factor = 8;
		case AxPipelineMode::SQRT: subsampling_factor = 10;
		case AxPipelineMode::LOG: subsampling_factor = 4;
		case AxPipelineMode::EIGHT_BIT: subsampling_factor = 4;
		case AxPipelineMode::JPEG_COMP: subsampling_factor = 1;
		default: subsampling_factor = 1;
		}

		// Set subsampling factor first
		//if (!setSubSampling(subsampling_factor))
			//return false;

		// Set pipeline mode next
		retval = axSetPipelineMode(pipeline_mode, AxChannelMode::CHAN_1, 0);
		if (retval != AxErr::NO_AxERROR)
		{
			dumpControlError(retval, pPreamble);
			return false;
		}

		char msg[256];
		sprintf(msg, "[Axsun Control] Pipeline mode is set to %s (subsampling factor: %d).", AxPipelineModeString(pipeline_mode).c_str(), subsampling_factor);
		SendStatusMessage(msg, false);
    }
    else
    {
        dumpControlError(retval, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
        return false;
    }

    return true;
}


bool AxsunControl::setSubSampling(uint8_t subsampling_factor)
{
	AxErr retval = AxErr::NO_AxERROR;
	const char* pPreamble = "[Axsun Control] Failed to set bypass mode: ";

	if (m_daq_device == AxDevType::EDAQ)
	{
		retval = axSetSubsamplingFactor(subsampling_factor, 0);
		if (retval != AxErr::NO_AxERROR)
		{
			dumpControlError(retval, pPreamble);
			return false;
		}

		char msg[256];
		sprintf(msg, "[Axsun Control] Sub-sampling mode is set (subsampling factor: %d).", subsampling_factor);
		SendStatusMessage(msg, false);
	}
	else
	{
		dumpControlError(retval, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}

	return true;
}


bool AxsunControl::setVDLHome()
{
	//std::unique_lock<std::mutex> lock(vdl_mutex);
	{
		AxErr retval = AxErr::NO_AxERROR;
		const char* pPreamble = "[Axsun Control] Failed to set VDL home: ";

		if (m_laser_device == AxDevType::LASER)
		{
			retval = axHomeVDL(0);
			if (retval != AxErr::NO_AxERROR)
			{
				dumpControlError(retval, pPreamble);
				return false;
			}
			SendStatusMessage("[Axsun Control] VDL length is set to home.", false);
		}
		else
		{
			dumpControlError(retval, pPreamble);
			SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
			return false;
		}
	}

	return true;
}


bool AxsunControl::setVDLLength(float position)
{
	//std::unique_lock<std::mutex> lock(vdl_mutex);
	{
		AxErr retval = AxErr::NO_AxERROR;
		const char* pPreamble = "[Axsun Control] Failed to set VDL length: ";

		if (m_laser_device == AxDevType::LASER)
		{
			retval = axMoveAbsVDL(position, 5, 0);
			if (retval != AxErr::NO_AxERROR)
			{
				dumpControlError(retval, pPreamble);
				return false;
			}

			char msg[256];
			sprintf(msg, "[Axsun Control] VDL length is set to %.2f mm.", position);
			SendStatusMessage(msg, false);
		}
		else
		{
			dumpControlError(retval, pPreamble);
			SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
			return false;
		}
	}

	return true;
}


void AxsunControl::getVDLStatus()
{
	AxErr retval = AxErr::NO_AxERROR;
	const char* pPreamble = "[Axsun Control] Failed to get VDL status: ";

	if (m_laser_device == AxDevType::LASER)
	{
		float current_pos, target_pos, speed; 
		int32_t error_from_last_home;
		uint8_t state, home_switch, limit_switch, VDL_error;

		retval = axGetVDLStatus(&current_pos, &target_pos, &speed, &error_from_last_home,
			&last_move_time, &state, &home_switch, &limit_switch, &VDL_error, 0);
		if (retval != AxErr::NO_AxERROR)
		{
			dumpControlError(retval, pPreamble);
			return;
		}

		printf("%f %f %f %d / %d %d %d %d %d\n", current_pos, target_pos, speed, error_from_last_home,
			last_move_time, state, home_switch, limit_switch, VDL_error);

		char msg[256];
		//sprintf(msg, "[Axsun Control] VDL length is set to %.2f mm.", position);
		SendStatusMessage(msg, false);
	}
	else
	{
		dumpControlError(retval, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return;
	}
}


bool AxsunControl::setClockDelay(uint32_t delay)
{
	AxErr retval = AxErr::NO_AxERROR;
	const char* pPreamble = "[Axsun Control] Failed to set clock delay: ";

	if (m_laser_device == AxDevType::LASER)
    {
		retval = axSetClockDelay(delay, 0);
		if (retval != AxErr::NO_AxERROR)
		{
			dumpControlError(retval, pPreamble);
			return false;
		}

		uint32_t delay0;
		axGetClockDelay(&delay0, 0);

		char msg[256];
        sprintf(msg, "[Axsun Control] Clock delay is set to %.3f nsec [%d].", CLOCK_GAIN * (double)delay + CLOCK_OFFSET, delay0);
		SendStatusMessage(msg, false);
	}
	else
	{
		dumpControlError(retval, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}

	return true;
}


bool AxsunControl::setDecibelRange(double min_dB, double max_dB)
{
	AxErr retval = AxErr::NO_AxERROR;
	const char* pPreamble = "[Axsun Control] Failed to set decibel range: ";

    double gain = 1275.0 * log10(2) / (max_dB - min_dB);
    double offset = -gain * (min_dB / 10.0 / log10(2) + 6.0);

	if ((offset < 127.996) && (offset > -128.000) && (gain > 0) && (gain < 15.9997))
	{
		if (m_daq_device == AxDevType::EDAQ)
		{
			retval = axSetEightBitOffset(offset, 0);
			if (retval != AxErr::NO_AxERROR)
			{
				dumpControlError(retval, pPreamble);
				return false;
			}
			retval = axSetEightBitGain(gain, 0);
			if (retval != AxErr::NO_AxERROR)
			{
				dumpControlError(retval, pPreamble);
				return false;
			}			

			char msg[256];
			sprintf(msg, "[Axsun Control] Decibel range is set to [%.1f %.1f] (offset: %.3f / gain: %.4f)", min_dB, max_dB, offset, gain);
			SendStatusMessage(msg, false);
		}
		else
		{
			dumpControlError(retval, pPreamble);
			SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
			return false;
		}
	}
	else
	{
		char msg[256];
        sprintf(msg, "[Axsun Control] Invalid decibel range... (offset: %.3f / gain: %.4f)", offset, gain);
		SendStatusMessage(msg, false);

        return false;
    }

    return true;
}

bool AxsunControl::getDeviceState()
{
	AxErr retval = AxErr::NO_AxERROR;
	const char* pPreamble = "[Axsun Control] Failed to get device state: ";

	if (m_daq_device == AxDevType::EDAQ) 
	{
		AxTECState TEC_state;
		retval = axGetTECState(&TEC_state, 1, 0);
		if (retval != AxErr::NO_AxERROR)
		{
			dumpControlError(retval, pPreamble);
			return false;
		}
		
		char msg[256];
		sprintf(msg, "[Axsun Control] %s", AxTECStateString(TEC_state).c_str());
		SendStatusMessage(msg, false);
	}
	else
	{		
		dumpControlError(retval, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}

	return true;
}



bool AxsunControl::writeFPGARegSingleBit(unsigned long regNum, int bitNum, bool set)
{
	AxErr retval = AxErr::NO_AxERROR;
	const char* pPreamble = "[Axsun Control] Failed to write FPGA single register bit: ";
	
	if (m_daq_device == AxDevType::EDAQ)
	{			
		unsigned short regVal = 0;
		retval = axGetFPGARegister(regNum, &regVal, 0);
		if (retval != AxErr::NO_AxERROR)
		{
			dumpControlError(retval, pPreamble);
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
		
		retval = axSetFPGARegister(regNum, regVal, 0);
		if (retval != AxErr::NO_AxERROR)
		{
			dumpControlError(retval, pPreamble);
			return false;
		}
	}
	else
	{
		dumpControlError(retval, pPreamble);
		SendStatusMessage("[Axsun Control] Unable to connect to the devices.", false);
		return false;
	}

	return true;
}


void AxsunControl::dumpControlError(AxErr res, const char* pPreamble)
{
    char msg[256] = { 0, };
    memcpy(msg, pPreamble, strlen(pPreamble));

	char err_msg[256];
	axGetErrorExplained(res, err_msg);
		
	if (res != AxErr::NO_AxERROR)
		strcat(msg, err_msg);
	else
		strcat(msg, "Connection failed.");

	SendStatusMessage(msg, true);
	axCloseAxsunOCTControl();
}
