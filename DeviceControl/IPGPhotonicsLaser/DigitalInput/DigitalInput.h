#ifndef DIGITAL_INPUT_H_
#define DIGITAL_INPUT_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class DigitalInput
{
public:
    explicit DigitalInput();
    virtual ~DigitalInput();

public:
    bool initialize();
    uint32_t start();
    void stop();

public:
	inline void setLineName(const char* _lineName) { lineName = _lineName; }

private:
	void dumpError(int res, const char* pPreamble);

private:		
    TaskHandle _taskHandle;
	const char* lineName;

public:
	callback2<const char*, bool> SendStatusMessage;
};

#endif // DIGITAL_INPUT_H_
