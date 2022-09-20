#ifndef _OCT_PROCESS_H_
#define _OCT_PROCESS_H_

#include <Havana3/Configuration.h>

#include <iostream>
#include <thread>
#include <complex>

#include <QString>
#include <QFile>

#include <ipps.h>
#include <ippvm.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <Common/array.h>
#include <Common/callback.h>
using namespace np;


struct FFT_C2C // 1D Fourier transformation for complex signal (for both forward and inverse transformation)
{
	FFT_C2C() :
        pFFTSpec(nullptr), pMemSpec(nullptr), pMemInit(nullptr), pMemBuffer(nullptr)
	{
	}

	~FFT_C2C()
	{
		if (pMemSpec) { ippsFree(pMemSpec); pMemSpec = nullptr; }
		if (pMemInit) { ippsFree(pMemInit); pMemInit = nullptr; }
		if (pMemBuffer) { ippsFree(pMemBuffer); pMemBuffer = nullptr; }
	}

	void forward(Ipp32fc* dst, const Ipp32fc* src)
	{
		ippsFFTFwd_CToC_32fc(src, dst, pFFTSpec, pMemBuffer);
	}

	void inverse(Ipp32fc* dst, const Ipp32fc* src)
	{
		ippsFFTInv_CToC_32fc(src, dst, pFFTSpec, pMemBuffer);
	}

	void initialize(int length)
	{
		const int ORDER = (int)(ceil(log2(length)));

		int sizeSpec, sizeInit, sizeBuffer;
		ippsFFTGetSize_C_32fc(ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuffer);

		pMemSpec = ippsMalloc_8u(sizeSpec);
		pMemInit = ippsMalloc_8u(sizeInit);
		pMemBuffer = ippsMalloc_8u(sizeBuffer);

		ippsFFTInit_C_32fc(&pFFTSpec, ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, pMemSpec, pMemInit);
	}

private:
	IppsFFTSpec_C_32fc* pFFTSpec;
    Ipp8u* pMemSpec;
    Ipp8u* pMemInit;
    Ipp8u* pMemBuffer;
};


class OCTProcess
{
// Methods
public: // Constructor & Destructor
	explicit OCTProcess(int nScans, int nAlines);
	~OCTProcess();
    
private: // Not to call copy constrcutor and copy assignment operator
    OCTProcess(const OCTProcess&);
    OCTProcess& operator=(const OCTProcess&);
	
public:
	// Generate OCT image
	void operator() (uint8_t* img, int16_t* fringe, float min, float max);
	   
	// For calibration
    void changeDiscomValue(int discom_val = 0);

// Variables
private:
    // FFT objects
    FFT_C2C ifft;
    
    // Size variables
    IppiSize raw_size;
    IppiSize fft_size;
	IppiSize fft2_size;
	        
    // OCT image processing buffer
    FloatArray2 signal;
    ComplexFloatArray2 complex_signal;
    ComplexFloatArray2 ifft_complex;    
    FloatArray2 ifft_linear;
	FloatArray2 ifft_log;
    
    // Calibration varialbes    
    ComplexFloatArray dispersion_win;
};

#endif
