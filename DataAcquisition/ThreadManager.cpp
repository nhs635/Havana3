
#include "ThreadManager.h"


ThreadManager::ThreadManager(const char* _threadID) :
    _running(false)
{
	memset(threadID, 0, MAX_LENGTH);
	memcpy(threadID, _threadID, strlen(_threadID));
}

ThreadManager::~ThreadManager()
{
    if (_thread.joinable())
    {
        _running = false;
        _thread.join();
    }
}


void ThreadManager::run()
{
    unsigned int frameIndex = 0;

    _running = true;
    while (_running)
        DidAcquireData(frameIndex++);
}

bool ThreadManager::startThreading()
{
    if (_thread.joinable())
    {
        char* msg = nullptr;
        sprintf(msg, "ERROR: %s thread is already running: ", threadID);
        dumpErrorSystem(-1, msg); //(::GetLastError(), msg);
        return false;
    }

    _thread = std::thread(&ThreadManager::run, this);
	
	char msg[256];
	sprintf(msg, "%s thread is started.", threadID);
	SendStatusMessage(msg, false);

    return true;
}

void ThreadManager::stopThreading()
{
    if (_thread.joinable())
    {
        DidStopData(); //_running = false;
        _thread.join();
    }

	char msg[256];
	sprintf(msg, "%s thread is finished normally.", threadID);
	SendStatusMessage(msg, false);
}


void ThreadManager::dumpErrorSystem(int res, const char* pPreamble)
{
    char* pErr = nullptr;
    char msg[MAX_LENGTH];
    memcpy(msg, pPreamble, strlen(pPreamble));

    sprintf(pErr, "Error code (%d)", res);
    strcat(msg, pErr);

    printf("%s\n", msg);
    SendStatusMessage(msg, true);
}

