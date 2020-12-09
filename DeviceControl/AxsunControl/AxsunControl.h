#ifndef AXSUN_CONTROL_H
#define AXSUN_CONTROL_H

#include <Common/callback.h>

#include <iostream>
#include <thread>

#import "AxsunOCTControl.tlb" named_guids raw_interfaces_only
using namespace AxsunOCTControl;

// Devices
#define DAQ_DEVICE           0
#define LASER_DEVICE         1

// Connection
#define DISCONNECTED         0
#define CONNECTED           -1

// Operation
#define ALINE_LENGTH		 1024
#define MAX_SAMPLE_LENGTH    2048
#define SAMPLE_LENGTH        1700
#define CLOCK_OFFSET         28.285
#define CLOCK_GAIN           0.576112

// Bypass Mode Enumerate
typedef enum bypass_mode {
	raw_adc_data = 0,
	windowed_data,
	post_fft,
	abs2,
	abs2_with_bg_subtracted,
	log_compressed,
	dynamic_range_reduced,
	jpeg_compressed
} bypass_mode;


class AxsunControl
{
// Methods
public: // Constructor & Destructor
    explicit AxsunControl();
    virtual ~AxsunControl();
	
public:
    // For Control
    bool initialize();
    bool setLaserEmission(bool status);
    bool setLiveImagingMode(bool status);
    ///bool setBurstRecording(unsigned short n_images, bool recording = true);
    bool setBackground(const unsigned short* pBackground);
    bool setDispersionCompensation(double a2 = 0, double a3 = 0, int length = SAMPLE_LENGTH);
	bool setBypassMode(bypass_mode _bypass_mode);
    bool setVDLHome();
    bool setVDLLength(float position);
    bool setClockDelay(unsigned long delay);
    bool setDecibelRange(double min_dB, double max_dB);
	bool getDeviceState();

private:
    bool writeFPGARegSingleBit(unsigned long regNum, int bitNum, bool set);
    bool setOffset(double offset);
    bool setGain(double gain);

    void dumpControlError(long res, const char* pPreamble);

// Variables
public:
	callback2<const char*, bool> SendStatusMessage;
	callback<int> DidTransferArray;

private:
    // For Control
    IAxsunOCTControlPtr m_pAxsunOCTControl;
    VARIANT_BOOL m_bIsConnected;
};

#endif
