#ifndef SIGNATEC_DAQ_H
#define SIGNATEC_DAQ_H

#include <Common/array.h>
#include <Common/callback.h>

#include <iostream>
#include <thread>
#include <chrono>

#define PX14_ADC_RATE		400 // MHz
#define PX14_VOLT_RANGE     1.2 // Vpp
#define PX14_BOOTBUF_IDX    1

#define MAX_MSG_LENGTH		2000


typedef struct _px14hs_* HPX14;

class SignatecDAQ
{
// Methods
public:
	explicit SignatecDAQ();
	virtual ~SignatecDAQ();
		
public:
	bool initialize();
	bool set_init();

	int getBootTimeBuffer(int idx);
	bool setBootTimeBuffer(int idx, int buffer_size);
    bool setDcOffset(int offset);

	bool startAcquisition();
	void stopAcquisition();

public:
	inline bool is_initialized() const { return !_dirty; }
	inline int getDataBufferSize() { return nChannels * nScans * nAlines / 4; }
	// Data buffer size (2 channel, half size. merge two buffers to build a frame) 
	
private:
	void run();

private:
	// Dump a PX14400 library error
	void dumpError(int res, const char* pPreamble);
	void dumpErrorSystem(int res, const char* pPreamble);

// Variables
private:
	// PX14400 board driver handle
	HPX14 _board;

	// DMA buffer pointer to acquire data
	unsigned short *dma_bufp;

	// Initialization flag
	bool _dirty;

	// thread
	std::thread _thread;
	
public:
	int nChannels, nScans, nAlines;
	unsigned int GainLevel;
	unsigned int AcqRate;
	unsigned int PreTrigger, TriggerDelay;
	unsigned int DcOffset;
	unsigned short BootTimeBufIdx;
	bool UseVirtualDevice, UseInternalTrigger;
	double frameRate;
	bool _running;	

public:
	// callbacks
	callback2<int, const np::Uint16Array2 &> DidAcquireData;
	callback<void> DidStopData;
	callback2<const char*, bool> SendStatusMessage;
};

#endif // SIGNATEC_DAQ_H_
