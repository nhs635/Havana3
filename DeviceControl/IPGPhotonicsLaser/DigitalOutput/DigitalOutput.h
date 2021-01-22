#ifndef DIGITAL_OUTPUT_H_
#define DIGITAL_OUTPUT_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class DigitalOutput
{
public:
    explicit DigitalOutput();
    virtual ~DigitalOutput();

public:
    bool initialize();
    void start();
    void stop();

public:
	inline void setLineName(const char* _lineName) { lineName = _lineName; }
	inline void setValue(uint32_t _value) { value = _value; }
	
private:
	void dumpError(int res, const char* pPreamble);

private:
	TaskHandle _taskHandle;

	uint32_t value;
	const char* lineName;

public:
    callback2<const char*, bool> SendStatusMessage;
};

#endif // DIGITAL_OUTPUT_H_
