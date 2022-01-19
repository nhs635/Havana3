#ifndef AXSUN_CONTROL_H
#define AXSUN_CONTROL_H

#include <AxsunOCTControl_LW_C.h>

#include <Common/array.h>
#include <Common/callback.h>

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>


// Operation
#define ALINE_LENGTH		 1024
#define NUM_ALINES		     1024
#define MAX_SAMPLE_LENGTH    2048
#define SAMPLE_LENGTH        1600
#define CLOCK_OFFSET         28.285
#define CLOCK_GAIN           0.576112

class AxsunControl
{
// Methods
public: // Constructor & Destructor
    explicit AxsunControl();
    virtual ~AxsunControl();
	
public:
	inline bool isInitialized() { return m_bInitialized; }

public:
    // For Control
    bool initialize();
    bool setLaserEmission(bool status);
    bool setLiveImagingMode(bool status);

    bool setBackground();
    bool setDispersionCompensation(double a2 = 0, double a3 = 0, int length = SAMPLE_LENGTH);
	bool setPipelineMode(AxPipelineMode pipeline_mode);
	bool setSubSampling(uint8_t subsampling_factor);
    bool setVDLHome();
    bool setVDLLength(float position);
	void getVDLStatus();
    bool setClockDelay(uint32_t delay);
    bool setDecibelRange(double min_dB, double max_dB);
	bool getDeviceState();

private:
    bool writeFPGARegSingleBit(unsigned long regNum, int bitNum, bool set);

private:
    void dumpControlError(AxErr res, const char* pPreamble);

// Variables
public:
	callback2<const char*, bool> SendStatusMessage;
	np::Uint16Array2 background_frame;
	std::vector<uint16_t> background_vector;

private:
	AxDevType m_daq_device;
	AxDevType m_laser_device;

	bool m_bInitialized;

	std::mutex vdl_mutex;
};

#endif
