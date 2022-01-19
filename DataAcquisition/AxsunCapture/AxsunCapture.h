#ifndef AXSUN_CAPTURE_H
#define AXSUN_CAPTURE_H

#include <Common\Array.h>
#include <Common\Callback.h>

#include <iostream>
#include <thread>

#include <AxsunOCTCapture.h>


// Capture
#define ENABLE_OPENGL_WINDOW false
#define PACKET_CAPACITY      2000


class AxsunCapture
{
// Methods
public: // Constructor & Destructor
	explicit AxsunCapture();
	virtual ~AxsunCapture();

private: // Not to call copy constrcutor and copy assignment operator
	AxsunCapture(const AxsunCapture&);
	AxsunCapture& operator=(const AxsunCapture&);

public:
	inline bool isInitialized() const { return !_dirty; }
    inline AOChandle getSession() const { return session; }

public:
	// For Capture
	bool initializeCapture();
	bool startCapture();
	void stopCapture();	

private:
	void captureRun();
	void dumpCaptureError(AxErr _retval, const char* pPreamble);
	void dumpCaptureErrorSystem(int32_t result, const char* pPreamble);

// Variables
private:
	// For Capture
	char capture_message[256];
    AOChandle session;
	std::thread _thread;

public:
	bool _dirty;
	bool capture_running;
    int image_width, image_height;
	uint32_t returned_image;
	np::Uint8Array2 image_data_out;
    uint32_t dropped_packets;
	double frameRate;

	// callbacks
	callback2<uint32_t, const np::Uint8Array2&> DidAcquireData;
	callback2<uint32_t, const np::Uint8Array2&> DidAcquireBG;
    callback<void> DidStopData;
	callback2<const char*, bool> SendStatusMessage;
};
#endif
