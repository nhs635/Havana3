#ifndef BASIC_FUNCTIONS_H
#define BASIC_FUNCTIONS_H

/* Basic Operation Functions for OCT Process */
#include <Common/array.h>
#include <Common/matrix.h>

#include <ipps.h>
#include <ippi.h>

namespace bf {

    inline void UnwrapPhase_32f(Ipp32f* p, int length)
    {
            Ipp32f* dp = ippsMalloc_32f(length - 1);
            Ipp32f* dps = ippsMalloc_32f(length - 1);
            Ipp32f* dp_corr = ippsMalloc_32f(length - 1);
            Ipp32f* cumsum = ippsMalloc_32f(length - 1);

            Ipp32f _pi = (Ipp32f)IPP_PI;
            Ipp32f cutoff = (Ipp32f)IPP_PI;

            // incremental phase variation
            // MATLAB: dp = diff(p,1,1);
            for (int i = 0; i < length - 1; i++)
                    dp[i] = p[i + 1] - p[i];

            // equivalent phase variation in [-pi, pi]
            // MATLAB: dps = mod(dp+pi,2*pi) - pi;
            for (int i = 0; i < length - 1; i++)
                    dps[i] = (dp[i] + _pi) - floor((dp[i] + _pi) / (2 * _pi)) * (2 * _pi) - _pi;

            // preserve variation sign for +pi vs. -pi
            // MATLAB: dps(dps==-pi & dp>0,:) = pi;
            for (int i = 0; i < length - 1; i++)
                    if ((dps[i] == -_pi) && (dp[i] > 0))
                            dps[i] = _pi;

            // incremental phase correction
            // MATLAB: dp_corr = dps - dp;
            for (int i = 0; i < length - 1; i++)
                    dp_corr[i] = dps[i] - dp[i];

            // Ignore correction when incremental variation is smaller than cutoff
            // MATLAB: dp_corr(abs(dp)<cutoff,:) = 0;
            for (int i = 0; i < length - 1; i++)
                    if ((fabs(dp[i]) < cutoff))
                            dp_corr[i] = 0;

            // Find cumulative sum of deltas
            // MATLAB: cumsum = cumsum(dp_corr, 1);
            cumsum[0] = dp_corr[0];
            for (int i = 1; i < length - 1; i++)
                    cumsum[i] = cumsum[i - 1] + dp_corr[i];

            // Integrate corrections and add to P to produce smoothed phase values
            // MATLAB: p(2:m,:) = p(2:m,:) + cumsum(dp_corr,1);
            for (int i = 1; i < length; i++)
                    p[i] += cumsum[i - 1];

            ippsFree(dp);
            ippsFree(dps);
            ippsFree(dp_corr);
            ippsFree(cumsum);

    }


    inline void LineSpace_32f(Ipp32f x0, Ipp32f x1, int length, Ipp32f* dst)
    {
            Ipp32f incre = (x1 - x0) / Ipp32f(length - 1);

            for (int i = 0; i < length; i++)
                    dst[i] = x0 + i * incre;
    }


    inline void Interpolation_32f(const Ipp32f* srcX, const Ipp32f* srcY, Ipp32f* dstX, int srcLen, int dstLen, Ipp32f* dstY)
    {
            // This function is only valid when diff(srcX) is always larger than 0...
            // Also diff(dstX) should be either >0 or <0.

            Ipp32f srcX_0 = 0, srcX_1 = 0;
            Ipp32f srcY_0 = 0, srcY_1 = 0;
            int ii;
            Ipp32f w;

            bool isIncre = (dstX[1] > dstX[0]) ? 1 : 0; //TRUE : FALSE;
            bool isExist;

            for (int i = 0; i < dstLen; i++)
            {
                    for (int j = 0; j < srcLen - 1; j++)
                    {
                            isExist = isIncre ? ((dstX[i] >= srcX[j]) && (dstX[i] <= srcX[j + 1])) : ((dstX[i] <= srcX[j]) && (dstX[i] >= srcX[j + 1]));
                            if (isExist)
                            {
                                    srcX_0 = srcX[j]; srcX_1 = srcX[j + 1];
                                    srcY_0 = srcY[j]; srcY_1 = srcY[j + 1];
                                    ii = j;
                                    break;
                            }
                    }

                    w = (dstX[i] - srcX_0) / (srcX_1 - srcX_0);
                    dstY[i] = (1 - w) * srcY_0 + w * srcY_1;
            }
    }


    inline void LinearInterp_32fc(const Ipp32fc* src, Ipp32fc* dst, int dst_len, Ipp32f* cal_ind, Ipp32f* cal_weight)
    {
		for (int i = 0; i < dst_len; i++)
		{
			dst[i].re = cal_weight[i] * src[(int)cal_ind[i]].re + (1 - cal_weight[i]) * src[(int)cal_ind[i] + 1].re;
			dst[i].im = cal_weight[i] * src[(int)cal_ind[i]].im + (1 - cal_weight[i]) * src[(int)cal_ind[i] + 1].im;
		}
    }


    inline void LinearRegression_32f(const Ipp32f* pSrcX, const Ipp32f* pSrcY, int length, Ipp32f& offset, Ipp32f& slope)
    {
            Ipp32f cov_xy, var_x;
            Ipp32f mean_x, mean_y;

            ippsMean_32f(pSrcX, length, &mean_x, ippAlgHintFast);
            ippsMean_32f(pSrcY, length, &mean_y, ippAlgHintFast);
            ippsDotProd_32f(pSrcX, pSrcY, length, &cov_xy);

            cov_xy = (cov_xy / (Ipp32f)length) - mean_x * mean_y;

            ippsDotProd_32f(pSrcX, pSrcX, length, &var_x);

            var_x = (var_x / (Ipp32f)length) - mean_x * mean_x;

            slope = cov_xy / var_x;
            offset = mean_y - slope * mean_x;
    }


    inline void transposeScaling_32f8u(const Ipp32f* src, Ipp8u* dst, IppiSize roi, float scale_min, float scale_max)
    {
        np::Array<uint8_t, 2> scale_temp(roi.width, roi.height);
        ippiScale_32f8u_C1R(src, roi.width * sizeof(float), scale_temp.raw_ptr(), roi.width * sizeof(uint8_t), roi, scale_min, scale_max);
        ippiTranspose_8u_C1R(scale_temp.raw_ptr(), roi.width * sizeof(uint8_t), dst, roi.height * sizeof(uint8_t), roi);
    }
	

  //  inline void polynomialFitting_32f(const float* src, float* dst, int length, int order)
  //  {
  //      Matrix X(length, order + 1);
  //      Matrix p(order + 1, 1);
  //      Matrix Y(length, 1);
		//Matrix Y1(length, 1);

		//for (int i = 0; i < length; i++)
		//{
		//	Y(i + 1, 1) = src[i];
		//	for (int j = 0; j < order + 1; j++)
		//		X(i + 1, j + 1) = (float)pow(i, j);
		//}
		//
		//p = Inv(Transpose(X) * X) * Transpose(X) * Y;
		//Y1 = X * p;
	
		//memcpy(dst, &Y1(1, 1), sizeof(float) * length);
  //  }


	inline void movingAverage_32f(const float* src, float* dst, int length, int winSize)
	{
		memcpy(dst, src, sizeof(float) * length);
		if (winSize % 2) // winSize should be odd.
		{
			int start_i = (winSize - 1) / 2;
			for (int i = start_i; i < length - start_i; i++)
			{
				float temp = 0;
				for (int j = -start_i; j <= start_i; j++)
					temp += src[i + j] / winSize;

				dst[i] = temp;
			}
		}
	}
}

#endif /* BASIC_FUNCTIONS_H */
