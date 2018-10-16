#ifndef GAIN_CONTROL_H_
#define GAIN_CONTROL_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class PmtGainControl
{
public:
	PmtGainControl();
	~PmtGainControl();

	double voltage;	

	bool initialize();
	void start();
	void stop();
		
public:
    callback2<const char*, bool> SendStatusMessage;

private:
	const char* physicalChannel;
	const char* sourceTerminal;

	TaskHandle _taskHandle;	
	void dumpError(int res, const char* pPreamble);
};

#endif // GAIN_CONTROL_H_
