
#include "OCTProcess.h"
#include <Common/basic_functions.h>


OCTProcess::OCTProcess(int nScans, int nAlines) :
    raw_size({ nScans, nAlines }),
	fft_size({ (int)exp2(ceil(log2((double)nScans))), nAlines }),
    fft2_size({ fft_size.width / 2, nAlines }),
	
	signal(fft_size.width, fft_size.height),
	complex_signal(fft_size.width, fft_size.height),
	ifft_complex(fft_size.width, fft_size.height),
    ifft_linear(fft2_size.width, fft2_size.height), 	
	ifft_log(fft2_size.width, fft2_size.height),
	    
    dispersion_win(raw_size.width)
{   
	ifft.initialize(fft_size.width);

	for (int i = 0; i < raw_size.width; i++)
	{
		float win = (float)(1 - cos(IPP_2PI * i / (raw_size.width - 1))) / 2; // Hann Window
		dispersion_win(i) = { 1 * win, 0 * win};
	}
}


OCTProcess::~OCTProcess()
{
}


/* OCT Image */
void OCTProcess::operator() (uint8_t* img, int16_t* fringe, float min, float max)
{	
	tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)raw_size.height),
		[&](const tbb::blocked_range<size_t>& r) {
		
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
			int r1 = raw_size.width * (int)i;
			int f1 = fft_size.width * (int)i;
			int f2 = fft2_size.width * (int)i;

			// 1. Single Precision Conversion
			ippsConvert_16s32f(fringe + r1, signal.raw_ptr() + r1, raw_size.width);
			ippsMulC_32f_I(4.0f, signal.raw_ptr() + r1, raw_size.width);

			// 2. Hanning Windowing & Dispersion Compensation
			ippsMul_32f32fc(signal.raw_ptr() + r1, (const Ipp32fc*)dispersion_win.raw_ptr(), (Ipp32fc*)complex_signal.raw_ptr() + r1, raw_size.width);
									
			// 3. Inverse Fourier Transform
			ifft.inverse((Ipp32fc*)ifft_complex.raw_ptr() + f1, (const Ipp32fc*)complex_signal.raw_ptr() + r1);

			// 4. dB Scaling
			ippsPowerSpectr_32fc((const Ipp32fc*)(ifft_complex.raw_ptr() + f1), ifft_linear.raw_ptr() + f2, fft2_size.width);
			ippsLog10_32f_A11(ifft_linear.raw_ptr() + f2, ifft_log + f2, fft2_size.width);
			ippsMulC_32f_I(10.0f, ifft_log + f2, fft2_size.width);
		}
	});

	// 5. 8-bit Scale
	ippiScale_32f8u_C1R(ifft_log, sizeof(float) * fft2_size.width, img, sizeof(uint8_t) * fft2_size.width, fft2_size, min, max);
}


/* OCT Calibration */
void OCTProcess::changeDiscomValue(int discom_val)
{
	double temp;
	float win;

	for (int i = 0; i < raw_size.width; i++)
	{
		win = (float)(1 - cos(IPP_2PI * i / (raw_size.width - 1))) / 2; // Hann Window

		temp = (((double)i - (double)raw_size.width / 2.0) / ((double)(raw_size.width) / 2.0)); 		
		dispersion_win(i) = { win * (float)cos((double)discom_val*temp*temp), win * (float)sin((double)discom_val*temp*temp) };
	}	
}