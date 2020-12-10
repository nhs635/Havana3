
#include "FreqDivider.h"

#include <iostream>

#include <NIDAQmx.h>
using namespace std;


FreqDivider::FreqDivider() :
    _taskHandle(nullptr),
    slow(4),
	sourceTerminal(""),
	counterChannel("")
{
}


FreqDivider::~FreqDivider()
{
    if (_taskHandle)
        DAQmxClearTask(_taskHandle);
}


bool FreqDivider::initialize()
{	
    SendStatusMessage("[SYNC] Initializing NI Counter for triggering FLIm laser...", false);

    int lowTicks = (int)(ceil(slow / 2));
    int highTicks = (int)(floor(slow / 2));
    uint64_t sampsPerChan = 120000;
    int res;

    if ((res = DAQmxCreateTask("", &_taskHandle)) != 0)
    {
        dumpError(res, "[SYNC ERROR] Failed to set NI Counter1: ");
        return false;
    }

	printf("%s %s\n", counterChannel, sourceTerminal);

    if ((res = DAQmxCreateCOPulseChanTicks(_taskHandle, counterChannel, NULL, sourceTerminal, DAQmx_Val_Low, 0, lowTicks, highTicks)) != 0)
    {
        dumpError(res, "[SYNC ERROR] Failed to set NI Counter2: ");
        return false;
    }

    if ((res = DAQmxCfgImplicitTiming(_taskHandle, DAQmx_Val_ContSamps, sampsPerChan)) != 0)
    {
        dumpError(res, "[SYNC ERROR] Failed to set NI Counter3: ");
        return false;
    }

    SendStatusMessage("[SYNC] NI Counter for triggering FLIm laser is successfully initialized.", false);

    return true;
}


void FreqDivider::start()
{
    if (_taskHandle)
    {
        SendStatusMessage("[SYNC] NI Counter is issueing external triggers for FLIm laser...", false);
        DAQmxStartTask(_taskHandle);
    }
}


void FreqDivider::stop()
{
    if (_taskHandle)
    {
        SendStatusMessage("[SYNC] NI Counter is stopped.", false);
        DAQmxStopTask(_taskHandle);
        DAQmxClearTask(_taskHandle);
        _taskHandle = nullptr;
    }
}


void FreqDivider::dumpError(int res, const char* pPreamble)
{	
    char errBuff[2048];
    if (res < 0)
        DAQmxGetErrorString(res, errBuff, 2048);

	char msg[2048];
	sprintf_s(msg, 2048, "%s %s", pPreamble, errBuff);
    SendStatusMessage(msg, true);

    if (_taskHandle)
    {
        DAQmxStopTask(_taskHandle);
        DAQmxClearTask(_taskHandle);
        _taskHandle = nullptr;
    }
}
