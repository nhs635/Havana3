
#include "AxsunCapture.h"

#include <windows.h> 

using namespace std;


AxsunCapture::AxsunCapture() :
	_dirty(true),
    capture_running(false),
    dataType(u8),
	image_width(1024),
	image_height(1024),
    returned_image(0)
{
}


AxsunCapture::~AxsunCapture()
{
	if (_thread.joinable())
	{
        capture_running = false;
		_thread.join();
	}
	axStopSession();
}


bool AxsunCapture::initializeCapture()
{
	int32_t result;
	const char* pPreamble = "[Axsun Catpure] Failed to initialize Axsun OCT capture session: ";

	//if (_dirty)
	//{
		// Start Axsun Session
		result = axStartSession(PACKET_CAPACITY);
		if (result != CAPTURE_OK)
		{
			dumpCaptureError(result, pPreamble);
			return false;
		}
		meta_data = np::Uint8Array(METADATA_BYTES);
		
		char msg[256]; 
		axGetMessage(capture_message);
		sprintf(msg, "[Axsun Catpure] %s", capture_message);
		SendStatusMessage("[Axsun Catpure] Session successfully started on network adapter", false);
		SendStatusMessage(msg, false);

		// Prevent the transition to Forced Trigger mode
		result = axSetTrigTimeout(100);
		if (result != CAPTURE_OK)
		{
			dumpCaptureError(result, pPreamble);
			return false;
		}

#if ENABLE_OPENGL_WINDOW
		// Setup OpenGL Window
		result = axSetupDisplay(0, 0, 0, 500, 500, NULL);
		if (result != CAPTURE_OK)
		{
			dumpCaptureError(result, pPreamble);
			return false;
		}
		SendStatusMessage("[Axsun Catpure] OpenGL display setup was successful.", false);
#endif

	//	_dirty = false;
	//}
		
	return true;
}


bool AxsunCapture::startCapture()
{
	int32_t result;
	const char* pPreamble = "[Axsun Catpure] Failed to start Axsun OCT capture session: ";

	// Capturing thread initiation
	if (_thread.joinable())
	{
		result = ::GetLastError();
		dumpCaptureError(result, pPreamble);
		return false;
	}

	_thread = std::thread(&AxsunCapture::captureRun, this); // thread executing
	if (::SetThreadPriority(_thread.native_handle(), THREAD_PRIORITY_TIME_CRITICAL) == 0)
	{
		result = ::GetLastError();
		dumpCaptureError(result, pPreamble);
		return false;
	}
	
	SendStatusMessage("Axsun Capture process is started.", false);

	return true;
}


void AxsunCapture::stopCapture()
{
	if (_thread.joinable())
	{
		DidStopData();
		_thread.join();
	}
	
	SendStatusMessage("Axsun Catpure process is finished normally.", false);
}


bool AxsunCapture::requestBufferedImage(np::Uint8Array2& image_data_out)
{
	int32_t result;
	const char* pPreamble = "[Axsun Capture] ";

	// define & initialize variables
	uint32_t ret_img_num = 0;
	uint32_t required_buffer_size = 0;
	int32_t height, width = 0;
	uint8_t force_trig_status, trig_too_fast_status = 0;
#if ENABLE_OPENGL_WINDOW
	request_mode req_mode = retrieve_and_display;
#else
	request_mode req_mode = retrieve_to_caller;
#endif

	// Start Axsun Session
	result = axStartSession(PACKET_CAPACITY);
	if (result != CAPTURE_OK)
	{
		dumpCaptureError(result, pPreamble);
		return false;
	}
	meta_data = np::Uint8Array(METADATA_BYTES);

	char msg[256];
	axGetMessage(capture_message);
	sprintf(msg, "[Axsun Catpure] %s", capture_message);
	SendStatusMessage("[Axsun Catpure] Session successfully started on network adapter", false);
	SendStatusMessage(msg, false);

	// Request an image
	int iter = 0;
	do
	{
		Sleep(0);

		result = axGetImageInfoAdv(-1, &ret_img_num, &height, &width, &dataType, &required_buffer_size,
			&force_trig_status, &trig_too_fast_status);			

	} while ((result != CAPTURE_OK) && (++iter < 100));

	if (iter == 100)
	{
		dumpCaptureError(result, pPreamble);
		return false;
	}

	if (result == CAPTURE_OK)
	{
		image_data_out = np::Uint8Array2(required_buffer_size / width, width);
		result = axRequestImageAdv(ret_img_num, image_data_out, meta_data, &height, &width, &dataType,
			required_buffer_size, 1, req_mode, &force_trig_status, &trig_too_fast_status);

		if (result == CAPTURE_OK)
		{
			char msg[256];
			sprintf(msg, "[Axsun Capture] Captured a single frame! (req_size: %d / height: %d / width: %d", required_buffer_size, height, width);
			SendStatusMessage(msg, false);
		}
		else
		{
			dumpCaptureError(result, pPreamble);
			return false;
		}
	}
	else
	{
		dumpCaptureError(result, pPreamble);
		return false;
	}

	result = axStopSession();
	if (result != CAPTURE_OK)
		dumpCaptureError(result, "[axStopSession] ");
	
	return true;
}



void AxsunCapture::captureRun()
{
	int32_t result;

	// define & initialize variables
    returned_image = 0;
    uint32_t required_buffer_size = 0;
    int32_t height, width = 0;
	uint8_t force_trig_status, trig_too_fast_status = 0;
#if ENABLE_OPENGL_WINDOW
    request_mode req_mode = retrieve_and_display;
#else
    request_mode req_mode = retrieve_to_caller;
#endif

	// Request & display images
	uint32_t loop_counter = 0; 

	image_data_out = np::Uint8Array2(image_height, 10 * image_width); // 10 frame buffers
	uint8_t *cur_section = nullptr;
	uint8_t *prev_section = nullptr;
	
	ULONG dwTickStart = 0, dwTickLastUpdate;

	uint64_t bytesAcquired = 0;
	uint32_t frameIndex = 0;
	
	capture_running = true;
    while (capture_running)
	{
		// get information about an image to be retreived from the main image buffer.
        result = axGetImageInfoAdv(-1, &returned_image, &height, &width, &dataType, &required_buffer_size,
            &force_trig_status, &trig_too_fast_status);
        if (result == CAPTURE_OK)
        {
			// Determine where new data transfer data will go.
			cur_section = &image_data_out(0, image_width * (loop_counter++ % 10));

			// if the image exists in the main image buffer, request for it to be rendered directly to the OpenGL window			
			if (width > image_width) required_buffer_size = height * image_width;
            result = axRequestImageAdv(returned_image, cur_section, meta_data, &height, &width, &dataType,
				required_buffer_size, 1, req_mode, &force_trig_status, &trig_too_fast_status);
			if (result == CAPTURE_OK)
			{
				prev_section = &image_data_out(0, (loop_counter % 10) * image_width);
			
				np::Uint8Array2 frame(prev_section, image_height, image_width);
				DidAcquireData(frameIndex++, frame);

				// Acquisition Status
				if (!dwTickStart)
					dwTickStart = dwTickLastUpdate = GetTickCount();

				// Update counters
				bytesAcquired += height * width;
				//loop_counter++;

				// Periodically update progress
				ULONG dwTickNow = GetTickCount();
				if (dwTickNow - dwTickLastUpdate > 5000)
				{
					double dRate;

					dwTickLastUpdate = dwTickNow;
					ULONG dwElapsed = dwTickNow - dwTickStart;

					if (dwElapsed)
					{
						dRate = (bytesAcquired / 1000.0) / (dwElapsed);

						char msg[256];
						sprintf(msg, "[Image#] %5d [Data Rate] %3.2f MB/s [Frame Rate] %.2f fps [%d %d]",
							frameIndex, dRate, (double)frameIndex / (double)(dwElapsed) * 1000.0, force_trig_status, trig_too_fast_status);
						SendStatusMessage(msg, false);
					}
				}
			}
        }
	}

	result = axStopSession();
	if (result != CAPTURE_OK)
		dumpCaptureError(result, "[axStopSession] ");
}


void AxsunCapture::dumpCaptureError(int32_t res, const char* pPreamble)
{
    char msg[256] = { 0, };
    memcpy(msg, pPreamble, strlen(pPreamble));

    char err[256] = { 0, };
    sprintf_s(err, 256, "Capture error code (%d)", res);
    strcat(msg, err);

    axGetMessage(capture_message);
	sprintf_s(err, 256, "%s (%s)", msg, capture_message);
	SendStatusMessage(err, true);

	int32_t result = axStopSession();
	if (result != CAPTURE_OK)
		dumpCaptureError(result, "[axStopSession] ");
}








///bool AxsunCapture::saveImages(const char* filepath)
///{
///    //int32_t result;
///    //const char* pPreamble = "[Axsun Capture] Failed to save recorded imaages: ";
///
///    //if (false) //(bypassMode == jpeg_compressed) || (bypassMode == dynamic_range_reduced))
///    //{
///    //    DidWriteImage();
///    //}
///    //else
///    //{
///    //    uint32_t packets_written = 0;
///    //    result = axSaveFile(filepath, 0, &packets_written);
///    //    if (result != CAPTURE_OK)
///    //    {
///    //        dumpCaptureError(result, pPreamble);
///    //        return false;
///    //    }
///    //}
///
///    //printf("[Axsun Capture] Recorded images are successfully saved.\n[%s]\n", filepath);
///
///    return true;
///}

