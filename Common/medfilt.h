#ifndef _MEDFILT_H_
#define _MEDFILT_H_

#include <ipps.h>
#include <ippi.h>
#include <ippcore.h>

class medfilt
{
public:
	medfilt()
	{
	};

	medfilt(int _width, int _height, int _kernelx, int _kernely) :
		width(_width), height(_height), kernelx(_kernelx), kernely(_kernely)
	{
		RoiSize = { width, height };
		MaskSize = { kernelx, kernely };

		ippiFilterMedianBorderGetBufferSize(RoiSize, MaskSize, ipp8u, 1, &BufferSize);
		MemBuffer8u = np::Array<uint8_t>(BufferSize);
		DstBuffer8u = np::Array<Ipp8u, 2>(width, height);

		ippiFilterMedianBorderGetBufferSize(RoiSize, MaskSize, ipp32f, 1, &BufferSize);
		MemBuffer32f = np::Array<uint8_t>(BufferSize);
		DstBuffer32f = np::Array<Ipp32f, 2>(width, height);
	};

	~medfilt()
	{
	};

	void operator() (Ipp8u* pSrcDst)
	{
		ippiFilterMedianBorder_8u_C1R(pSrcDst, sizeof(Ipp8u) * RoiSize.width, DstBuffer8u, sizeof(Ipp8u) * RoiSize.width, RoiSize, MaskSize, ippBorderRepl, 0, MemBuffer8u);
		memcpy(pSrcDst, DstBuffer8u, sizeof(Ipp8u) * width * height);
	};

	void operator() (Ipp32f* pSrcDst)
	{
		ippiFilterMedianBorder_32f_C1R(pSrcDst, sizeof(Ipp32f) * RoiSize.width, DstBuffer32f, sizeof(Ipp32f) * RoiSize.width, RoiSize, MaskSize, ippBorderRepl, 0, MemBuffer32f);
		memcpy(pSrcDst, DstBuffer32f, sizeof(Ipp32f) * width * height);
	};

private:
	int width, height, kernelx, kernely;
	IppiSize RoiSize, MaskSize;
	Ipp32s BufferSize;
	np::Array<uint8_t> MemBuffer8u;
	np::Array<uint8_t> MemBuffer32f;
	np::Array<Ipp8u, 2> DstBuffer8u;
	np::Array<Ipp32f, 2> DstBuffer32f;
};

#endif
