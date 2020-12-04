
#include "PmtGainControl.h"
#include <Havana3/Configuration.h>

#ifdef NI_ENABLE

#include <iostream>

#include <NIDAQmx.h>
using namespace std;


PmtGainControl::PmtGainControl() :
    _taskHandle(nullptr),
    voltage(0),
    physicalChannel(NI_PMT_GAIN_CHANNEL)
{
}


PmtGainControl::~PmtGainControl()
{
    stop();
}


bool PmtGainControl::initialize()
{
    SendStatusMessage("Initializing NI Analog Output for PMT gain control...", false);

	int res;
	double data[1] = { voltage };

	/*********************************************/
	// Voltage Generator
	/*********************************************/
	if ((res = DAQmxCreateTask("", &_taskHandle)) != 0)
	{
		dumpError(res, "ERROR: Failed to set Gain Control1: ");
		return false;
	}
	if ((res = DAQmxCreateAOVoltageChan(_taskHandle, physicalChannel, "", 0.0, 1.0, DAQmx_Val_Volts, NULL)) != 0)
	{
		dumpError(res, "ERROR: Failed to set Gain Control2: ");
		return false;
	}	
	if ((res = DAQmxWriteAnalogF64(_taskHandle, 1, TRUE, DAQmx_Val_WaitInfinitely, DAQmx_Val_GroupByChannel, data, NULL, NULL)) != 0)
	{
		dumpError(res, "ERROR: Failed to set Gain Control3: ");
		return false;
	}		

    SendStatusMessage("NI Analog Output for PMT gain control is successfully initialized.", false);

	return true;
}


void PmtGainControl::start()
{
	if (_taskHandle)
	{
        SendStatusMessage("PMT gain control generates a voltage...", false);
		DAQmxStartTask(_taskHandle);
	}
}


void PmtGainControl::stop()
{
	if (_taskHandle)
	{
		double data[1] = { 0.0 };
		DAQmxWriteAnalogF64(_taskHandle, 1, TRUE, DAQmx_Val_WaitInfinitely, DAQmx_Val_GroupByChannel, data, NULL, NULL);

        SendStatusMessage("NI Analog Output is stopped.", false);
		DAQmxStopTask(_taskHandle);
		DAQmxClearTask(_taskHandle);
		
		_taskHandle = nullptr;
	}
}


void PmtGainControl::dumpError(int res, const char* pPreamble)
{	
	char errBuff[2048];
	if (res < 0)
		DAQmxGetErrorString(res, errBuff, 2048);

    SendStatusMessage(((QString)pPreamble + (QString)errBuff).toUtf8().data(), true);

	if (_taskHandle)
	{
		double data[1] = { 0.0 };
		DAQmxWriteAnalogF64(_taskHandle, 1, TRUE, DAQmx_Val_WaitInfinitely, DAQmx_Val_GroupByChannel, data, NULL, NULL);

		DAQmxStopTask(_taskHandle);
		DAQmxClearTask(_taskHandle);
		
		_taskHandle = nullptr;
	}
}

#endif
