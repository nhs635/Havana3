#ifndef FREQ_DIVIDER_H_
#define FREQ_DIVIDER_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class FreqDivider
{
public:
    FreqDivider();
    ~FreqDivider();

    int slow;
    const char* sourceTerminal;
    const char* counterChannel;

    bool initialize();
    void start();
    void stop();

public:
    callback2<const char*, bool> SendStatusMessage;

private:
    TaskHandle _taskHandle;
    void dumpError(int res, const char* pPreamble);
};

#endif // FREQ_DIVIDER_H_
