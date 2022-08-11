
#include "AxsunCapture.h"

#include <windows.h> 

using namespace std;


AxsunCapture::AxsunCapture() :
	session(nullptr),
	_dirty(true),
    capture_running(false),    
    image_width(1024),
    image_height(1024),
    returned_image(0),
	frameRate(0.0)
{
}


AxsunCapture::~AxsunCapture()
{
	if (_thread.joinable())
	{
        capture_running = false;
		_thread.join();
	}
	if (session)
		axStopSession(session);
}


bool AxsunCapture::initializeCapture()
{
	AxErr retval;
	const char* pPreamble = "[Axsun Catpure] Failed to initialize Axsun OCT capture session: ";

	//if (_dirty)
	//{
		// Start Axsun Session
        retval = axStartSession(&session, PACKET_CAPACITY);
		if (retval != AxErr::NO_AxERROR)
		{
			dumpCaptureError(retval, pPreamble);
			return false;
        }

		// Select capture interface for this session
		retval = axSelectInterface(session, AxInterface::GIGABIT_ETHERNET);
		if (retval != AxErr::NO_AxERROR)
		{
			dumpCaptureError(retval, pPreamble);
			return false;
		}

        // Get status message
        char msg[256];
        axGetMessage(session, capture_message);
        sprintf(msg, "[Axsun Catpure] %s", capture_message);
        SendStatusMessage("[Axsun Catpure] Session successfully started on network adapter", false);
        SendStatusMessage(msg, false);

        // Prevent the transition to Forced Trigger mode
        retval = axSetTrigTimeout(session, 100);
        if (retval != AxErr::NO_AxERROR)
        {
            dumpCaptureError(retval, pPreamble);
            return false;
        }

#if ENABLE_OPENGL_WINDOW
		// Setup OpenGL Window
		retval = axSetupDisplay(session, 0, 0, 0, 500, 500, NULL);
		if (retval != AxErr::NO_AxERROR)
		{
			dumpCaptureError(retval, pPreamble);
			return false;
		}
		retval = axAdjustBrightnessContrast(session, 1, 0, 1);
		if (retval != AxErr::NO_AxERROR)
		{
			dumpCaptureError(retval, pPreamble);
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
		dumpCaptureErrorSystem(result, pPreamble);
		return false;
	}

	_thread = std::thread(&AxsunCapture::captureRun, this); // thread executing
	if (::SetThreadPriority(_thread.native_handle(), THREAD_PRIORITY_TIME_CRITICAL) == 0)
	{
		result = ::GetLastError();
		dumpCaptureErrorSystem(result, pPreamble);
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


void AxsunCapture::captureRun()
{
	AxErr retval;
	const char* pPreamble = "[Axsun Catpure] Failed to continue Axsun OCT capture session: ";

	// define & initialize variables
	uint32_t imaging;
    uint32_t last_packet, last_frame, last_image = 0, last_image0 = -1;
	uint32_t frames_since_sync;

	image_info_t info{};
	request_prefs_t prefs{};
    prefs.request_mode = AxRequestMode::RETRIEVE_TO_CALLER;

	// Request & display images
	uint32_t loop_counter = 0; 

	image_data_out = np::Uint8Array2(image_height, 4 * image_width); // 4 frame buffers
	uint8_t *cur_section = nullptr;
	
	ULONG dwTickStart = 0, dwTickLastUpdate;

	uint64_t bytesAcquired = 0, bytesAcquiredUpdate = 0;
	uint32_t frameIndex = 0, frameIndexUpdate = 0;
	
	capture_running = true;
    while (capture_running)
	{
		// get status information
        retval = axGetStatus(session, &imaging, &last_packet, &last_frame, &last_image, &dropped_packets, &frames_since_sync);
        if (retval != AxErr::NO_AxERROR)
        {
            dumpCaptureError(retval, pPreamble);
            return;
        }
		
		// only new image allowed
        if (last_image == last_image0)
            continue;
		last_image0 = last_image;

		// get information about an image to be retreived from the main image buffer.
        if (imaging)
        {
            retval = axGetImageInfo(getSession(), MOST_RECENT_IMAGE, &info);
            if (retval == AxErr::NO_AxERROR)
            {
				/// // Background capture
				///if (info.data_type != AxDataType::U8)
				///{
				///	np::Uint8Array2 frame(2 * info.height, info.width);
				///	axRequestImage(session, info.image_number, prefs, 2 * info.height * info.width, frame.raw_ptr(), &info);
	
				///	DidAcquireBG(frameIndex++, frame);
				///	DidAcquireData(frameIndex, frame);
				///	continue;
				///}

                // Determine where new data transfer data will go.
                cur_section = &image_data_out(0, image_width * (loop_counter % 4));

                // if no errors, configure the request preferences and then request the image for display
                retval = axRequestImage(session, info.image_number, prefs, 2 * image_width * image_height, cur_section, &info);
//                if (retval == AxErr::NO_AxERROR)
                {
                    np::Uint8Array2 frame(cur_section, info.height, image_width);
                    DidAcquireData(frameIndex++, frame);
                    frameIndexUpdate++;

                    // Acquisition Status
                    if (!dwTickStart)
                        dwTickStart = dwTickLastUpdate = GetTickCount();

                    // Update counters
                    bytesAcquired += info.height * info.width;
                    bytesAcquiredUpdate += info.height * info.width;
                    loop_counter++;

                    // Periodically update progress
                    ULONG dwTickNow = GetTickCount();
                    if (dwTickNow - dwTickLastUpdate > 2500)
                    {
                        double dRate, dRateUpdate;

                        ULONG dwElapsed = dwTickNow - dwTickStart;
                        ULONG dwElapsedUpdate = dwTickNow - dwTickLastUpdate;
                        dwTickLastUpdate = dwTickNow;

                        if (dwElapsed)
                        {
                            dRate = double(bytesAcquired / 1024.0 / 1024.0) / double(dwElapsed / 1000.0);
                            dRateUpdate = double(bytesAcquiredUpdate / 1024.0 / 1024.0) / double(dwElapsedUpdate / 1000.0);
                            frameRate = (double)frameIndexUpdate / (double)(dwElapsedUpdate) * 1000.0;

                            char msg[256];
                            sprintf(msg, "[Axsun Catpure] [Image#] %5d [Data Rate] %3.2f MiB/s [Frame Rate] %.2f fps [%d %d]",
                                frameIndex, dRateUpdate, frameRate, info.force_trig, info.trig_too_fast);
                            SendStatusMessage(msg, false);
                            sprintf(msg, "imaging: %d, width: %d, last_packet: %d, last_frame: %d, last_image: %d, dropped_packets: %d, frame_since_sync: %d",
                                imaging, info.width, last_packet, last_frame, last_image, dropped_packets, frames_since_sync);
                            SendStatusMessage(msg, false);
                        }

                        // Reset
                        frameIndexUpdate = 0;
                        bytesAcquiredUpdate = 0;
                    }
                }
            }
        }

		// loop timer for approximately 50 fps update rate
		std::this_thread::sleep_for(0ms);
	}

	retval = axStopSession(session);
	if (retval != AxErr::NO_AxERROR)
		dumpCaptureError(retval, "[axStopSession] ");
	else
		session = nullptr;
}


void AxsunCapture::dumpCaptureError(AxErr _retval, const char* pPreamble)
{
    char msg[256] = { 0, };
    memcpy(msg, pPreamble, strlen(pPreamble));

    char err[256] = { 0, };
    sprintf_s(err, 256, "Capture error code (%d)", _retval);
    strcat(msg, err);

    axGetMessage(session, capture_message);
	sprintf_s(err, 256, "%s (%s)", msg, capture_message);
	SendStatusMessage(err, true);

	AxErr retval = axStopSession(session);
	if (retval != AxErr::NO_AxERROR)
		dumpCaptureError(retval, "[axStopSession] ");
	else
		session = nullptr;
}


void AxsunCapture::dumpCaptureErrorSystem(int32_t res, const char* pPreamble)
{
	char msg[256] = { 0, };
	memcpy(msg, pPreamble, strlen(pPreamble));

	char err[256] = { 0, };
	sprintf_s(err, 256, "Capture error code (%d)", res);
	strcat(msg, err);

	sprintf_s(err, 256, "%s (%s)", msg, "system error");
	SendStatusMessage(err, true);

	AxErr retval = axStopSession(session);
	if (retval != AxErr::NO_AxERROR)
		dumpCaptureError(retval, "[axStopSession] ");
	else
		session = nullptr;
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

