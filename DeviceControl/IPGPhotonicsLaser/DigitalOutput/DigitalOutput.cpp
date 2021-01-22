
#include "DigitalOutput.h"

#include <NIDAQmx.h>
using namespace std;


DigitalOutput::DigitalOutput() :
	_taskHandle(nullptr),
    value(0b00000000),
    lineName("")
{
}


DigitalOutput::~DigitalOutput()
{
    if (_taskHandle)
        DAQmxClearTask(_taskHandle);
}


bool DigitalOutput::initialize()
{
    SendStatusMessage("[DIO] Initializing NI Digital Output...", false);

    int res;
    int32 written;    

    if ((res = DAQmxCreateTask("", &_taskHandle)) != 0)
    {
        dumpError(res, "[DIO ERROR] Failed to initialize NI Digital Output: ");
        return false;
    }
    if ((res = DAQmxCreateDOChan(_taskHandle, lineName, "", DAQmx_Val_ChanForAllLines)) != 0)
    {
        dumpError(res, "[DIO ERROR] Failed to initialize NI Digital Output: ");
        return false;
    }
    if ((res = DAQmxWriteDigitalU32(_taskHandle, 1, TRUE, 10.0, DAQmx_Val_GroupByChannel, (const uInt32*)&value, &written, NULL)) != 0)
    {
        dumpError(res, "[DIO ERROR] Failed to initialize NI Digital Output: ");
        return false;
    }

    SendStatusMessage("[DIO] NI Digital Output is successfully initialized.", false);

    return true;
}


void DigitalOutput::start()
{
    if (_taskHandle)
    {
        SendStatusMessage("[DIO] Digital voltage is applied to the lines...", false);
        DAQmxStartTask(_taskHandle);        
    }
}


void DigitalOutput::stop()
{
    if (_taskHandle)
    {
        SendStatusMessage("[DIO] NI Digital Output is stopped.", false);
        DAQmxStopTask(_taskHandle);
        DAQmxClearTask(_taskHandle);
        _taskHandle = nullptr;
    }
}


void DigitalOutput::dumpError(int res, const char* pPreamble)
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
