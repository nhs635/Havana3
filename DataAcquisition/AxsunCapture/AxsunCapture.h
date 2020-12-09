#ifndef AXSUN_CAPTURE_H
#define AXSUN_CAPTURE_H

#include <iostream>
#include <thread>

#include "AxsunOCTCapture.h"

#include "Common\Array.h"
#include "Common\Callback.h"

// Capture
#define ENABLE_OPENGL_WINDOW false
#define CAPTURE_OK           1
#define PACKET_CAPACITY      1000000
#define METADATA_BYTES       34
#define FIXED_WIDTH			 256
#define SYNC_WIDTH			 1024
#define BUFFER_NUMBER


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

public:
	// For Capture
	bool initializeCapture();
	bool startCapture();
	void stopCapture();
	bool requestBufferedImage(np::Uint8Array2& image_data_out);
	//bool saveImages(const char* filepath);

private:
	void captureRun();
	void dumpCaptureError(int32_t res, const char* pPreamble);

// Variables
private:
	// For Capture
	char capture_message[256];
	np::Uint8Array meta_data;
	std::thread _thread;

public:
	bool _dirty;
	bool capture_running;
	data_type dataType;
	int image_width;
	int image_height;
	uint32_t returned_image;
	np::Uint8Array2 image_data_out;
	double frameRate;

	// callbacks
	callback2<uint32_t, const np::Uint8Array2&> DidAcquireData;
	callback<void> DidWriteImage;
    callback<void> DidStopData;
	callback2<const char*, bool> SendStatusMessage;
};


#endif
