#ifndef GAIN_CONTROL_H_
#define GAIN_CONTROL_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class PmtGainControl
{
public:
	explicit PmtGainControl();
	virtual ~PmtGainControl();

public:
	bool initialize();
	void start();
	void stop();
	
public:
	inline void setVoltage(double _voltage) { voltage = _voltage; }
	inline double getVoltage() { return voltage; }
	inline void setPhysicalChannel(const char* _physicalChannel) { physicalChannel = _physicalChannel; }

private:
	void dumpError(int res, const char* pPreamble);

private:
	TaskHandle _taskHandle;

	double voltage;
	const char* physicalChannel;

public:
	callback2<const char*, bool> SendStatusMessage;
};

#endif // GAIN_CONTROL_H_
