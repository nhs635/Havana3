#ifndef FLIM_PROCESS_H
#define FLIM_PROCESS_H

#include <Havana3/Configuration.h>

#ifndef FLIM_CH_START_5
#error("FLIM_CH_START_5 is not defined for FLIM processing.");
#endif
#ifndef GAUSSIAN_FILTER_WIDTH
#error("GAUSSIAN_FILTER_WIDTH is not defined for FLIM processing.");
#endif
#ifndef GAUSSIAN_FILTER_STD
#error("GAUSSIAN_FILTER_STD is not defined for FLIM processing.");
#endif
#ifndef FLIM_SPLINE_FACTOR
#error("FLIM_SPLINE_FACTOR is not defined for FLIM processing.");
#endif
#ifndef INTENSITY_THRES
#error("INTENSITY_THRES is not defined for FLIM processing.");
#endif

#include <iostream>
#include <vector>
#include <utility>
#include <cmath>

#include <QString>
#include <QFile>

#include <ipps.h>
#include <ippi.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <mkl_service.h>
#include <mkl_df.h>

#include <Common/array.h>
#include <Common/callback.h>
using namespace np;


struct FLIM_PARAMS
{
    float bg;

    float samp_intv = 1.0f;
    float width_factor = 2.0f;

    int ch_start_ind[5] = { 0, };
    float delay_offset[3] = { 0.0f, };
};

struct FILTER // Gaussian Filtering
{
    FILTER() :
        pSpec(nullptr), pBuf(nullptr), pTaps(nullptr)
    {
    }

    ~FILTER()
    {
        if (pSpec) { ippsFree(pSpec); pSpec = nullptr; }
        if (pBuf) { ippsFree(pBuf); pBuf = nullptr; }
        if (pTaps) { ippsFree(pTaps); pTaps = nullptr; }
    }

    void operator() (Ipp32f* pDst, Ipp32f* pSrc, int y)
    {
        ippsFIRSR_32f(pSrc, pDst, srcWidth, pSpec, NULL, NULL, &pBuf[srcWidth * y]);
    }

    void initialize(int _tapsLen, int _srcWidth, int ny)
    {
        tapsLen = _tapsLen;
        srcWidth = _srcWidth;

        if (pTaps) { ippsFree(pTaps); pTaps = nullptr; }
        pTaps = ippsMalloc_32f(tapsLen);
        float x;
        for (int i = 0; i < tapsLen; i++)
        {
            x = ((float)i - ((float)tapsLen - 1) / 2) / (float)GAUSSIAN_FILTER_STD;
            pTaps[i] = std::exp(-0.5f * x * x);
        }
        float sum; ippsSum_32f(pTaps, tapsLen, &sum, ippAlgHintFast);
        ippsDivC_32f_I(sum, pTaps, tapsLen);

        ippsFIRSRGetSize(tapsLen, ipp32f, &specSize, &bufSize);
        if (pSpec) { ippsFree(pSpec); pSpec = nullptr; }
        pSpec = (IppsFIRSpec_32f*)ippsMalloc_8u(specSize);
        if (pBuf) { ippsFree(pBuf); pBuf = nullptr; }
        pBuf = ippsMalloc_8u(ny * bufSize);
        ippsFIRSRInit_32f(pTaps, tapsLen, ippAlgDirect, pSpec);
    }

private:
    int tapsLen;
    int srcWidth;
    int specSize, bufSize;
    IppsFIRSpec_32f* pSpec;
    Ipp8u* pBuf;
    Ipp32f* pTaps;
};

struct RESIZE
{
public:
    RESIZE() : scoeff(nullptr), pSeq(nullptr), pMask(nullptr), nx(-1), initiated(false)
    {
        memset(start_ind, 0, sizeof(int) * 4);
        memset(end_ind, 0, sizeof(int) * 4);
    }

    ~RESIZE()
    {
        if (scoeff) delete[] scoeff;
        if (pSeq) ippsFree(pSeq);
        if (pMask) ippsFree(pMask);
    }

    void operator() (const Uint16Array2 &src, const FLIM_PARAMS &pParams)
    {
        // 0. Initialize
        int _nx = pParams.ch_start_ind[4] - pParams.ch_start_ind[0];
        if ((nx != _nx) || !initiated)
            initialize(pParams, _nx, FLIM_SPLINE_FACTOR, src.size(1));
		int roi_len = (int)round(pulse_roi_length / ActualFactor);

        // 1. Crop ROI
        int offset = pParams.ch_start_ind[0]; // + pParams.pre_trig;
        ippiConvert_16u32f_C1R(&src(offset, 0), sizeof(uint16_t) * src.size(0),
                               crop_src.raw_ptr(), sizeof(float) * crop_src.size(0), srcSize);

        // 2. BG subtraction
		ippsSubC_32f_I(pParams.bg, crop_src.raw_ptr(), crop_src.length());

        ///int ch_ind4[5]; memcpy(ch_ind4, pParams.ch_start_ind, sizeof(int) * 5);
        ///int dc_determine_len = 5;

		// Parallel-for loop
        memcpy(mask_src.raw_ptr(), crop_src.raw_ptr(), sizeof(float) * mask_src.length());
        tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)ny),
            [&](const tbb::blocked_range<size_t>& r) {
            for (size_t i = r.begin(); i != r.end(); ++i)
            {
				float max_val; int max_ind;

				// 3. Jitter compensation - is it valid?
				//int cpos = 4, rpos = 4;
				//ippsMaxIndx_32f(&mask_src(0, (int)i), start_ind[0], &max_val, &cpos);

				//int offset = cpos - rpos;
				//if (offset < 0) offset += mask_src.size(0);
				//std::rotate(&mask_src(0, (int)i), &mask_src(offset, (int)i), &mask_src(mask_src.size(0) - 1, (int)i));

                ///int end_ind4[4]; memcpy(end_ind4, ch_ind4 + 1, sizeof(int) * 4);
                ///ippsSubC_32s_ISfs(ch_ind4[0], end_ind4, 4, 0);
				
				// 4. Remove artifact manually (smart artifact removal method)
                for (int ch = 0; ch < 4; ch++)
                {
                    if (start_ind[ch])
                    {
                        if (ch == 0)
							ippsSet_32f(0.0f, &mask_src(start_ind[ch], (int)i), end_ind[ch] - start_ind[ch] + 1);
                        else
                        {
							ippsMaxIndx_32f(&mask_src(start_ind[ch], (int)i), end_ind[ch] - start_ind[ch] + 1, &max_val, &max_ind);
							max_ind += start_ind[ch] - 1;
							///end_ind4[ch - 1] = max_ind - 4;
							ippsSet_32f(0.0f, &mask_src(max_ind - 2, (int)i), 7);
                        }
                    }
                }
				
				// 5. Determine whether saturated
				Ipp32f thres = 61000.0 - pParams.bg;
				ippsThreshold_32f(&mask_src(0, (int)i), &sat_src(0, (int)i), sat_src.size(0), thres, ippCmpLess);
				ippsSubC_32f_I(thres, &sat_src(0, (int)i), sat_src.size(0));
				for (int j = 1; j < 4; j++)
				{
					saturated((int)i, j) = 0;
					offset = pParams.ch_start_ind[j] - pParams.ch_start_ind[0];
					ippsSum_32f(&sat_src(offset, (int)i), roi_len, &saturated((int)i, j), ippAlgHintFast);
				}
				
				/// 5. DC level auto-adjustment
				///				for (int ch = 1; ch < 4; ch++)
				///				{
				///					float dc_level;
				///					ippsMean_32f(&mask_src(end_ind4[ch] - dc_determine_len, (int)i), dc_determine_len, &dc_level, ippAlgHintFast);
				///					if (mask_src.size(0) == (ch_ind4[4] - ch_ind4[0]))
				///						ippsSubC_32f_I(dc_level, &mask_src(ch_ind4[ch] - ch_ind4[0], (int)i), ch_ind4[ch + 1] - ch_ind4[ch]);
				///				}

                // 6. Up-sampling by cubic natural spline interpolation
                DFTaskPtr task1;
                dfsNewTask1D(&task1, nx, x, DF_UNIFORM_PARTITION, 1, &mask_src(0, (int)i), DF_MATRIX_STORAGE_ROWS);
                dfsEditPPSpline1D(task1, DF_PP_CUBIC, DF_PP_NATURAL, DF_BC_NOT_A_KNOT, 0, DF_NO_IC, 0, scoeff + (int)i * (nx - 1) * DF_PP_CUBIC, DF_NO_HINT);
                dfsConstruct1D(task1, DF_PP_SPLINE, DF_METHOD_STD);
                dfsInterpolate1D(task1, DF_INTERP, DF_METHOD_PP, nsite, x, DF_UNIFORM_PARTITION, 1, &dorder,
                                 DF_NO_APRIORI_INFO, &ext_src(0, (int)i), DF_MATRIX_STORAGE_ROWS, NULL);
                dfDeleteTask(&task1);

                // 7. Software broadening by FIR Gaussian filtering
                _filter(&filt_src(0, (int)i), &ext_src(0, (int)i), (int)i);
            }
        });

		mkl_free_buffers();
    }

    void initialize(const FLIM_PARAMS& pParams, int _nx, int _upSampleFactor, int _alines)
    {
        /* Parameters */
        nx = _nx; ny = _alines;
        upSampleFactor = _upSampleFactor;
        nsite = nx * upSampleFactor;
        ActualFactor = (float)(nx * upSampleFactor - 1) / (float)(nx - 1);
        x[0] = 0.0f; x[1] = (float)nx - 1.0f;
        srcSize = { (int)nx, (int)ny };
        dorder = 1;

        /* Find pulse roi length for mean delay calculation */
        for (int i = 0; i < 5; i++)
            ch_start_ind1[i] = (int)round((float)pParams.ch_start_ind[i] * ActualFactor);

        int diff_ind[4];
        for (int i = 0; i < 4; i++)
            diff_ind[i] = ch_start_ind1[i + 1] - ch_start_ind1[i];
		diff_ind[3] = round((FLIM_CH_START_5 - 1) * ActualFactor);

        ippsMin_32s(diff_ind, 4, &pulse_roi_length);
		//char msg[256];
		//sprintf(msg, "FLIm Initializing... %d", pulse_roi_length);
		//SendStatusMessage(msg);

        /* sequence for mean delay caculation */
        if (pSeq) { ippsFree(pSeq); pSeq = nullptr; }
        pSeq = ippsMalloc_32f(pulse_roi_length);
        ippsVectorSlope_32f(pSeq, pulse_roi_length, 0, 1);

        /* mask for removal of rotary junction artifacts */
        if (pMask) { ippsFree(pMask); pMask = nullptr; }
        pMask = ippsMalloc_32f(nx);
        ippsSet_32f(1.0f, pMask, nx);
        memset(start_ind, 0, sizeof(int) * 4);
        memset(end_ind, 0, sizeof(int) * 4);

        /* Array of spline coefficients */
        if (scoeff) { delete[] scoeff; scoeff = nullptr; }
        scoeff = new float[ny * (nx - 1) * DF_PP_CUBIC];

        /* data buffer allocation */
        crop_src = std::move(FloatArray2((int)nx, (int)ny));
        mask_src = std::move(FloatArray2((int)nx, (int)ny));
        sat_src  = std::move(FloatArray2((int)nx, (int)ny));
        ext_src  = std::move(FloatArray2((int)nsite, (int)ny));
        filt_src = std::move(FloatArray2((int)nsite, (int)ny));

        saturated = std::move(FloatArray2((int)ny, 4));
        memset(saturated, 0, sizeof(float) * saturated.length());

        /* filter coefficient allocation */
        _filter.initialize(GAUSSIAN_FILTER_WIDTH, nsite, ny);

        initiated = true;
    }

private:
    IppiSize srcSize;
    float* scoeff;
    float x[2];
    MKL_INT dorder;

public:
    bool initiated;

    MKL_INT nx, ny; // original data length, dimension
    MKL_INT nsite; // interpolated data length

    int ch_start_ind1[5];
    int upSampleFactor;
    Ipp32f ActualFactor;
    int pulse_roi_length;

    FILTER _filter;

    FloatArray2 saturated;

    Ipp32f* pSeq;
    Ipp32f* pMask;
    int start_ind[4];
    int end_ind[4];

    FloatArray2 crop_src;
    FloatArray2 mask_src;
    FloatArray2 sat_src;
    FloatArray2 ext_src;
    FloatArray2 filt_src;

	//callback<const char*> SendStatusMessage;
};

struct INTENSITY
{
public:
    INTENSITY() : _ny(0) {}
    ~INTENSITY() {}

    void operator() (const RESIZE& resize) // scans x 256 ==> 256 x 4
    {
        if (_ny != resize.ny)
        {
            intensity = FloatArray2(resize.ny, 4);
            _ny = resize.ny;
        }

        int offset;
        for (int i = 0; i < 4; i++)
        {
			offset = resize.ch_start_ind1[i] - resize.ch_start_ind1[0];
            for (int j = 0; j < intensity.size(0); j++)
            {
                intensity(j, i) = 0;
                if (resize.saturated(j, i) < 4)
                    ippsSum_32f(&resize.ext_src(offset, j), resize.pulse_roi_length, &intensity(j, i), ippAlgHintFast);
            }
        }

        for (int i = 0; i < 4; i++)
            ippsDiv_32f_I(&intensity(0, 0), &intensity(0, 3 - i), intensity.size(0));
    }

public:
    int _ny;
    np::FloatArray2 intensity;
};

struct LIFETIME
{
public:
    LIFETIME() : _ny(0) {}
    ~LIFETIME() {}

    void operator() (const RESIZE& resize, const FLIM_PARAMS& pParams, FloatArray2& intensity)
    {
        if (_ny != resize.ny)
        {
            mean_delay = FloatArray2(resize.ny, 4);
            lifetime = FloatArray2(resize.ny, 3);
            _ny = resize.ny;
        }

        tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)resize.ny),
            [&](const tbb::blocked_range<size_t>& r) {
            for (size_t i = r.begin(); i != r.end(); ++i)
            {
                for (int j = 0; j < 4; j++)
                {
                    int offset, width, left, maxIdx;
                    offset = resize.ch_start_ind1[j] - resize.ch_start_ind1[0];

                    // 1. Get IRF width
                    const float* pulse = &resize.filt_src(offset, (int)i);
                    WidthIndex_32f(pulse, 0.5f, resize.pulse_roi_length, maxIdx, width);
                    roiWidth[j] = (int)round(pParams.width_factor * width);
                    left = (int)floor(roiWidth[j] / 2);

                    // 2. Get mean delay of each channel (iterative process)
                    MeanDelay_32f(resize, offset, (int)i, maxIdx, roiWidth[j], left, mean_delay((int)i, j));
                    mean_delay((int)i, j) = (mean_delay((int)i, j) + (float)resize.ch_start_ind1[j]) / resize.ActualFactor;
                }

                // 3. Subtract mean delay of IRF to mean delay of each channel
                for (int j = 0; j < 3; j++)
                {
                    if (intensity((int)i, j + 1) > INTENSITY_THRES)
                        lifetime((int)i, j) = pParams.samp_intv * (mean_delay((int)i, j + 1) - mean_delay((int)i, 0)) - pParams.delay_offset[j];
                    else
                        lifetime((int)i, j) = 0.0f;
                }
            }
        });
    }

    void WidthIndex_32f(const Ipp32f* src, Ipp32f th, Ipp32s length, Ipp32s& maxIdx, Ipp32s& width)
    {
        Ipp32s left0 = 0, right0 = 0;
        Ipp32f maxVal;

        ippsMaxIndx_32f(src, length, &maxVal, &maxIdx);

        for (Ipp32s i = maxIdx; i >= 0; i--)
        {
            if (src[i] < maxVal * th)
            {
                left0 = i;
                break;
            }
        }
        for (Ipp32s i = maxIdx; i < length; i++)
        {
            if (src[i] < maxVal * th)
            {
                right0 = i;
                break;
            }
        }
        width = right0 - left0 + 1;
    }

    void MeanDelay_32f(const RESIZE& resize, int offset, int aline, Ipp32s maxIdx, Ipp32s width, Ipp32s left, Ipp32f &mean_delay)
    {
        Ipp32f sum, weight_sum;
        int start;

        mean_delay = (float)maxIdx;

        for (int i = 0; i < 10; i++)
        {
            start = (int)round(mean_delay) - left;

            if ((start < 0) || (start + width > resize.pulse_roi_length))
            {
                mean_delay = 0;
                break;
            }

            const float* pulse = &resize.filt_src(offset + start, aline);
            sum = 0; weight_sum = 0;

            if (resize.pSeq)
            {
                ippsDotProd_32f(pulse, &resize.pSeq[start], width, &weight_sum);
                ippsSum_32f(pulse, width, &sum, ippAlgHintFast);
            }

            if (sum)
                mean_delay = weight_sum / sum;
            else
            {
                mean_delay = 0;
                break;
            }

            if ((mean_delay > resize.pulse_roi_length) || (mean_delay < 0))
            {
                mean_delay = 0;
                break;
            }
        }
    }

public:
    int roiWidth[4];
    int _ny;
    FloatArray2 mean_delay;
    FloatArray2 lifetime;
};


class FLImProcess
{
// Methods
public: // Constructor & Destructor
    explicit FLImProcess();
    ~FLImProcess();

private: // Not to call copy constrcutor and copy assignment operator
    FLImProcess(const FLImProcess&);
    FLImProcess& operator=(const FLImProcess&);
	
public:
    // Generate fluorescence intensity & lifetime
    void operator()(FloatArray2& intensity, FloatArray2& mean_delay, FloatArray2& lifetime, Uint16Array2& pulse);
	///void getResize(Uint16Array2& pulse);
	///void getIntensity(FloatArray2& intensity);
	///void getLifetime(FloatArray2& mean_delay, FloatArray2& lifetime);

    // For FLIM parameters setting
    void setParameters(Configuration* pConfig);

    // For masking
    void saveMaskData(QString maskpath = "flim_mask.dat");
    void loadMaskData(QString maskpath = "flim_mask.dat");

// Variables
public:
    FLIM_PARAMS _params;

    RESIZE _resize; // resize objects
    INTENSITY _intensity; // intensity objects
    LIFETIME _lifetime; // lifetime objects

public:
	// Callbacks
	callback2<const char*, bool> SendStatusMessage;
};

#endif
