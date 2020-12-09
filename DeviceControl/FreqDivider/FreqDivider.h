#ifndef FREQ_DIVIDER_H_
#define FREQ_DIVIDER_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class FreqDivider
{
public:
	explicit FreqDivider();
    virtual ~FreqDivider();

public:
    bool initialize();
    void start();
    void stop();

public:
	inline void setSlow(int _slow) { slow = _slow; }
	inline void setSourceTerminal(const char* _sourceTerminal) { sourceTerminal = _sourceTerminal; }
	inline void setCounterChannel(const char* _counterChannel) { counterChannel = _counterChannel; }

private:
	void dumpError(int res, const char* pPreamble);

private:
	TaskHandle _taskHandle;

	int slow;
	const char* sourceTerminal;
	const char* counterChannel;
	
public:
	callback2<const char*, bool> SendStatusMessage;
};

#endif // FREQ_DIVIDER_H_
