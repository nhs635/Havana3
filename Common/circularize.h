#ifndef _CIRCULARIZE_H_
#define _CIRCULARIZE_H_

#include <ipps.h>
#include <ippi.h>
#include <ippcore.h>

#include <chrono>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include "array.h"


class circularize
{
public:
	circularize()
	{
        }

	circularize(int _radius, int _alines, bool half = true)
	{
		radius = _radius;
		alines = _alines;

		np::Array<float, 2> x_map, y_map;

		if (half)
		{
			// Generate Cicularize Map
			diameter = radius;
			x_map = np::Array<float, 2>(diameter, diameter);
			y_map = np::Array<float, 2>(diameter, diameter);
			
			Ipp32f* horizontal_line = ippsMalloc_32f(diameter);
			Ipp32f* vertical_line = ippsMalloc_32f(diameter);
			ippsVectorSlope_32f(horizontal_line, diameter, (Ipp32f)+radius, -2.0f);

			for (int i = 0; i < diameter; i++)
			{
				ippsSet_32f((float)(2 * i - radius), vertical_line, diameter);
				memcpy(x_map.raw_ptr() + i * diameter, horizontal_line, sizeof(float) * diameter);
				memcpy(y_map.raw_ptr() + i * diameter, vertical_line, sizeof(float) * diameter);
			}
			ippFree(horizontal_line);
			ippFree(vertical_line);
		}
		else
		{
			// Generate Cicularize Map
			diameter = 2 * radius;
			x_map = np::Array<float, 2>(diameter, diameter);
			y_map = np::Array<float, 2>(diameter, diameter);

			Ipp32f* horizontal_line = ippsMalloc_32f(diameter);
			Ipp32f* vertical_line = ippsMalloc_32f(diameter);
			ippsVectorSlope_32f(horizontal_line, diameter, (Ipp32f)+radius, -1.0f);

			for (int i = 0; i < diameter; i++)
			{
				ippsSet_32f((float)(i - radius), vertical_line, diameter);
				memcpy(x_map.raw_ptr() + i * diameter, horizontal_line, sizeof(float) * diameter);
				memcpy(y_map.raw_ptr() + i * diameter, vertical_line, sizeof(float) * diameter);
			}
			ippFree(horizontal_line);
			ippFree(vertical_line);
		}

		// Rho : Interpolation Map
		rho = np::Array<float, 2>(diameter, diameter);
		ippsMagnitude_32f(x_map, y_map, rho, diameter * diameter);
		ippsMulC_32f_I(((Ipp32f)radius - 1.0f) / radius, rho, diameter * diameter);

		// Theta : Interpolation Map
		theta = np::Array<float, 2>(diameter, diameter);
		ippsPhase_32f(x_map, y_map, theta, diameter * diameter);
		//ippsMulC_32f_I(1.0f, theta, diameter * diameter);
		ippsAddC_32f_I((Ipp32f)IPP_PI, theta, diameter * diameter);
		ippsMulC_32f_I(((Ipp32f)alines - 1.0f) / (Ipp32f)IPP_2PI, theta, diameter * diameter);
        }

	~circularize()
	{

        }
	
public:
	void operator() (np::Array<float, 2>& rect_im, np::Array<float, 2>& circ_im, int offset = 0)
	{
		IppiSize srcSize = { radius, alines }; // width * height
		IppiRect srcRoi = { 0, 0, radius, alines };
		IppiSize dstRoiSize = { diameter, diameter };

		ippiRemap_32f_C1R(&rect_im(offset, 0), srcSize, sizeof(Ipp32f) * rect_im.size(0), srcRoi,
			rho, sizeof(Ipp32f) * dstRoiSize.width, theta, sizeof(Ipp32f) * dstRoiSize.width,
			circ_im.raw_ptr(), sizeof(Ipp32f) * dstRoiSize.width, dstRoiSize, IPPI_INTER_LINEAR);	
        }

	void operator() (np::Array<uint8_t, 2>& rect_im, uint8_t* circ_im, int offset = 0)
	{
		IppiSize srcSize = { radius, alines }; // width * height
		IppiRect srcRoi = { 0, 0, radius, alines };
		IppiSize dstRoiSize = { diameter, diameter };

		ippiRemap_8u_C1R(&rect_im(offset, 0), srcSize, sizeof(Ipp8u) * rect_im.size(0), srcRoi,
			rho, sizeof(Ipp32f) * dstRoiSize.width, theta, sizeof(Ipp32f) * dstRoiSize.width,
			circ_im, sizeof(Ipp8u) * dstRoiSize.width, dstRoiSize, IPPI_INTER_LINEAR);
        }

	void operator() (np::Array<uint8_t, 2>& rect_im, uint8_t* circ_im, const char* vertical, int offset = 0)
	{
		IppiSize srcSize = { alines, radius }; // width * height
		IppiRect srcRoi = { 0, 0, alines, radius };
		IppiSize dstRoiSize = { diameter, diameter };

		ippiRemap_8u_C1R(&rect_im(0, offset), srcSize, sizeof(Ipp8u) * rect_im.size(0), srcRoi,
			theta, sizeof(Ipp32f) * dstRoiSize.width, rho, sizeof(Ipp32f) * dstRoiSize.width,
			circ_im, sizeof(Ipp8u) * dstRoiSize.width, dstRoiSize, IPPI_INTER_LINEAR);

        (void)vertical;
	}

	void operator() (np::Array<uint8_t, 2>& rect_im, uint8_t* circ_im, const char* vertical, const char* rgb, int offset = 0)
	{
		IppiSize srcSize = { alines, radius }; // width * height
		IppiRect srcRoi = { 0, 0, alines, radius };
		IppiSize dstRoiSize = { diameter, diameter };

		ippiRemap_8u_C3R(&rect_im(0, offset), srcSize, sizeof(Ipp8u) * rect_im.size(0), srcRoi,
			theta, sizeof(Ipp32f) * dstRoiSize.width, rho, sizeof(Ipp32f) * dstRoiSize.width,
			circ_im, sizeof(Ipp8u) * 3 * dstRoiSize.width, dstRoiSize, IPPI_INTER_LINEAR);
		
		(void)vertical;
		(void)rgb;
	}
		
public:
	int alines, radius, diameter;
private:
	np::Array<float, 2> rho;
	np::Array<float, 2> theta;
};

#endif


