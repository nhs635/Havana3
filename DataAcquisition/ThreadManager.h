#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include <iostream>
#include <thread>

#include <Common/SyncObject.h>
#include <Common/callback.h>

#define MAX_LENGTH 2000

class ThreadManager
{
public:
    explicit ThreadManager(const char* _threadID = "");
    virtual ~ThreadManager();

public:
    // thread operation
	callback<int> DidAcquireData;
	callback<void> DidStopData;
	callback2<const char*, bool> SendStatusMessage;

private:
    void run(); // run thread operation
    std::thread _thread;

public:
    bool _running;
    bool startThreading();
    void stopThreading();

private:
    void dumpErrorSystem(int res, const char* pPreamble);

private:
    char threadID[MAX_LENGTH];

};

#endif // THREADMANAGER_H
