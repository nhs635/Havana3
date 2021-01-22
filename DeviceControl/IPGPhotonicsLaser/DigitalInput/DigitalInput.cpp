
#include "DigitalInput.h"

#include <NIDAQmx.h>
using namespace std;


DigitalInput::DigitalInput() :
	_taskHandle(nullptr),
	lineName("")    
{
}


DigitalInput::~DigitalInput()
{
    if (_taskHandle)
        DAQmxClearTask(_taskHandle);
}


bool DigitalInput::initialize()
{
    SendStatusMessage("[DIO] Initializing NI Digital Input...", false);

    int res;

    if ((res = DAQmxCreateTask("", &_taskHandle)) != 0)
    {
        dumpError(res, "[DIO ERROR] Failed to initialize NI Digital Input: ");
        return false;
    }
    if ((res = DAQmxCreateDIChan(_taskHandle, lineName, "", DAQmx_Val_ChanForAllLines)) != 0)
    {
        dumpError(res, "[DIO ERROR] Failed to initialize NI Digital Input: ");
        return false;
    }
    if ((res = DAQmxStartTask(_taskHandle)))
    {
        dumpError(res, "[DIO ERROR] Failed to initialize NI Digital Input: ");
        return false;
    }

    SendStatusMessage("[DIO] NI Digital Input is successfully initialized.", false);

    return true;
}


uint32_t DigitalInput::start()
{
    if (_taskHandle)
    {
        SendStatusMessage("[DIO] Digital voltage applied to the lines is reading...", false);

        int res;
        uInt32 value;
        int32 read;
        if ((res = DAQmxReadDigitalU32(_taskHandle, 1, 10.0, DAQmx_Val_GroupByChannel, &value, 1, &read, NULL)))
        {
            dumpError(res, "[DIO ERROR] Failed to read NI Digital Input: ");
            return 0;
        }
        return value;
    }
    else
    {
        return 0;
    }
}


void DigitalInput::stop()
{
    if (_taskHandle)
    {
        SendStatusMessage("[DIO] NI Digital Input is stopped.", false);
        DAQmxStopTask(_taskHandle);
        DAQmxClearTask(_taskHandle);
        _taskHandle = nullptr;
    }
}


void DigitalInput::dumpError(int res, const char* pPreamble)
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
