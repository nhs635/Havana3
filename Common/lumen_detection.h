#ifndef LUMEN_DETECTION_H
#define LUMEN_DETECTION_H

#include <iostream>
#include <fstream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <Common/array.h>
#include <Common/basic_functions.h>

#include <ipps.h>
#include <ippi.h>
#include <ippcore.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#define NORM_BG_LEVEL		8.0f
#define NORM_MEAN_LEVEL		32.0f

using namespace std;
using namespace cv;


class LumenDetection
{
public:
	explicit LumenDetection(int _outer_sheath, int _inner_offset, bool _data_save = false,
		bool _ref_sup = false, int _ref_dist = 0, float _ref_level = 0.0f,
		int _gw_th = 50, float _gwp_th = -0.5f, int _max_w_th = 80) :
		outer_sheath(_outer_sheath), inner_offset(_inner_offset),
		ref_sup(_ref_sup), ref_dist(_ref_dist), ref_level(_ref_level),
		gw_th(_gw_th), gwp_th(_gwp_th), max_w_th(_max_w_th), data_save(_data_save)
	{
	}

	~LumenDetection()
	{
	}

public:
	void operator()(np::Uint8Array2& src, np::FloatArray& contour)
	{
		preprocessing(src);
		guide_wire_detection();
		delineate_contour(contour);
	}

	void preprocessing(np::Uint8Array2& src)
	{
		// Gaussian blurring
		cv::Mat input = cv::Mat(src.size(1), src.size(0), CV_8UC1, src.raw_ptr());
		cv::Mat output = cv::Mat(src.size(1), src.size(0), CV_8UC1);
		GaussianBlur(input, output, cv::Size(9, 9), 2.5);
		preprop = np::FloatArray2(src.size(0), src.size(1));
		ippsConvert_8u32f(output.data, preprop, preprop.length());

		// Suppressing reflection artifact (if needed)
		if (ref_sup)
		{
			np::FloatArray2 reflection_temp(preprop.size(0), preprop.size(1));
			ippiCopy_32f_C1R(&preprop(ref_dist, 0), sizeof(float) * preprop.size(0),
				&reflection_temp(0, 0), sizeof(float) * reflection_temp.size(0),
				{ preprop.size(0) - ref_dist, preprop.size(1) });
			ippsMulC_32f_I(ref_level, reflection_temp, reflection_temp.length());
			ippsSub_32f_I(reflection_temp, preprop, preprop.length());
		}
		
		// Set size
		nradius = preprop.size(1);
		nalines = preprop.size(0);

		// Normalization
		ippiMean_32f_C1R(&preprop(nradius - inner_offset - 30, 0), sizeof(float) * preprop.size(0), { 15, preprop.size(1) }, &bg, ippAlgHintAccurate);
		ippiMean_32f_C1R(&preprop(0, 0), sizeof(float) * preprop.size(0), { nradius - inner_offset - 30, preprop.size(1) }, &mean, ippAlgHintAccurate);

		ippsSubC_32f_I(bg, preprop, preprop.length());
		ippsAddC_32f_I(8.0f, preprop, preprop.length());
		ippsMulC_32f_I(32.0f / (mean - bg + 8.0f), preprop, preprop.length());
			
		/****************************************************************/
		if (data_save)
		{
			std::ofstream fout;
			fout.open("preprop.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)preprop.raw_ptr(), sizeof(float) * preprop.length());
				fout.close();
			}
		}
		/****************************************************************/
	}

	void guide_wire_detection()
	{
		// Calculate summation profile along aline direction
		np::FloatArray sum_profile(2 * nalines);
		for (int i = 0; i < nalines; i++)
		{
			ippsMean_32f(&preprop(outer_sheath / 2, i), nradius - inner_offset - outer_sheath / 2, &sum_profile(i), ippAlgHintFast);
			sum_profile(i + nalines) = sum_profile(i);
		}
		std::rotate(&sum_profile(0), &sum_profile(nalines / 2), &sum_profile(2 * nalines));
		
		// Moving averaging	
		np::FloatArray sum_profile1(2 * nalines);
		memcpy(sum_profile1, sum_profile, sizeof(float) * sum_profile.length());
		for (int i = 1; i < 2 * nalines - 1; i++)
		{
			ippsSum_32f(&sum_profile(i - 1), 3, &sum_profile1(i), ippAlgHintAccurate);
			sum_profile1(i) = sum_profile1(i) / 3;
		}
		
		/****************************************************************/
		if (data_save)
		{
			std::ofstream fout;
			fout.open("sum_profile1.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)sum_profile1.raw_ptr(), sizeof(float) * sum_profile1.length());
				fout.close();
			}
		}
		/****************************************************************/

		// Differentiation
		np::FloatArray sum_diff(2 * nalines), sum_diff2(2 * nalines);
		memset(sum_diff, 0, sizeof(float) * sum_diff.length());
		memset(sum_diff2, 0, sizeof(float) * sum_diff2.length());
		ippsSub_32f(&sum_profile1(1), &sum_profile1(0), sum_diff, 2 * nalines - 1);
		ippsSub_32f(&sum_diff(1), &sum_diff(0), sum_diff2, 2 * nalines - 2);
		
		/****************************************************************/
		if (data_save)
		{
			std::ofstream fout;
			fout.open("sum_diff.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)sum_diff.raw_ptr(), sizeof(float) * sum_diff.length());
				fout.close();
			}
		}
		if (data_save)
		{
			std::ofstream fout;
			fout.open("sum_diff2.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)sum_diff2.raw_ptr(), sizeof(float) * sum_diff2.length());
				fout.close();
			}
		}
		/****************************************************************/

		// Peak & valley detection
		std::vector<int> peaks, valleys;
		for (int i = 0; i < 2 * nalines; i++)
		{
			if ((sum_diff(i) * sum_diff(i + 1) < 0) || ((sum_diff(i + 1) == 0) && (sum_diff(i) * sum_diff(i+2) < 0)))
			{
				if (sum_diff(i) <= 0)
					peaks.push_back(i + 1);
				else if (sum_diff(i) >= 0)
					valleys.push_back(i + 1);
			}
		}

		/****************************************************************/
		if (data_save)
		{
			std::ofstream fout;
			fout.open("peaks.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)&peaks[0], sizeof(int) * peaks.size());
				fout.close();
			}
		}
		if (data_save)
		{
			std::ofstream fout;
			fout.open("valleys.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)&valleys[0], sizeof(int) * valleys.size());
				fout.close();
			}
		}
		/****************************************************************/

		// Find guide wire position
		float max_p;
		ippsMax_32f(sum_profile1, sum_profile1.length(), &max_p);
		int max_ps = (int)(0.9f * max_p);		
		gw_th = (max_ps > gw_th) ? gw_th : max_ps;

		std::vector<int> gw_peaks;
		for (int i = 0; i < peaks.size(); i++)
		{
			if ((peaks.at(i) >= nalines / 2) && (peaks.at(i) < 3 * nalines / 2))
			{
				int fwhm = _find_fwhm(sum_profile1, peaks.at(i));
				//sum_profile1(peaks.at(i)) > gw_th

				if ((fwhm < 30) && ((sum_diff2(peaks.at(i)) < gwp_th) || (sum_diff2(peaks.at(i) - 1) < gwp_th) || (sum_diff2(peaks.at(i) - 2) < gwp_th)))
					gw_peaks.push_back(peaks.at(i));
			}
		}

		// Find guire-wire valleys	
		std::vector<int> gw_valleys_left, gw_valleys_right;
		for (int i = 0; i < gw_peaks.size(); i++)
		{
			int gwp = gw_peaks.at(i);

			// Left valley
			{
				std::vector<int> gw_valley_cands;
				std::vector<float> gw_valley_vals;
				for (int j = 0; j < valleys.size(); j++)
				{
					int v = valleys.at(j);
					if ((v < gwp) && (v > gwp - max_w_th))
					{
						gw_valley_cands.push_back(v);
						gw_valley_vals.push_back(sum_profile1(v));
					}
				}
				auto mi = std::distance(std::begin(gw_valley_vals), std::min_element(std::begin(gw_valley_vals), std::end(gw_valley_vals)));
				gw_valleys_left.push_back(gw_valley_cands[mi]);
			}

			// Right valley
			{
				std::vector<int> gw_valley_cands;
				std::vector<float> gw_valley_vals;
				for (int j = 0; j < valleys.size(); j++)
				{
					int v = valleys.at(j);
					if ((v > gwp) && (v < gwp + max_w_th))
					{
						gw_valley_cands.push_back(v);
						gw_valley_vals.push_back(sum_profile1(v));
					}
				}
				auto mi = std::distance(std::begin(gw_valley_vals), std::min_element(std::begin(gw_valley_vals), std::end(gw_valley_vals)));
				gw_valleys_right.push_back(gw_valley_cands[mi]);
			}
		}

		// Find guide-wire region
		std::vector<int> gw_region_left, gw_region_right;
		int min_p_th = gw_th / 5;

		// In case of non-bright guide-wire
		if (gw_peaks.size() == 0)
		{
			for (int i = 0; i < valleys.size(); i++)
			{
				if ((valleys.at(i) >= nalines / 2) && (valleys.at(i) < 3 * nalines / 2))
				{
					if (sum_profile1(valleys.at(i)) < 12.0f)
					{
						gw_valleys_left.push_back(valleys.at(i));
						gw_valleys_right.push_back(valleys.at(i));
					}
				}
			}
		}

		// Left edge
		for (int i = 0; i < gw_valleys_left.size(); i++)
		{			
			int gwv = gw_valleys_left.at(i);

			std::vector<int> gw_edge_cands;
			for (int j = 0; j < peaks.size(); j++)
			{
				int p = peaks.at(j);
				if ((p < gwv) && (p > gwv - max_w_th) && (sum_profile1(p) > (sum_profile1(gwv) + min_p_th)))
					gw_edge_cands.push_back(p);
			}

			if (gw_edge_cands.size() > 0)
				gw_region_left.push_back(gw_edge_cands.back() - nalines / 2);
		}

		// Right edge
		for (int i = 0; i < gw_valleys_right.size(); i++)
		{
			int gwv = gw_valleys_right.at(i);

			std::vector<int> gw_edge_cands;
			for (int j = 0; j < peaks.size(); j++)
			{
				int p = peaks.at(j);
				if ((p > gwv) && (p < gwv + max_w_th) && (sum_profile1(p) > (sum_profile1(gwv) + min_p_th)))
					gw_edge_cands.push_back(p);
			}

			if (gw_edge_cands.size() > 0)
				gw_region_right.push_back(gw_edge_cands.front() - nalines / 2);
		}

		// Remove guide-wire regions
		int gw_width = 0;
		if (gw_region_left.size() == gw_region_right.size())
			for (int i = 0; i < gw_region_left.size(); i++)
				gw_width += (gw_region_right.at(i) - gw_region_left.at(i) + 1);
		if (gw_width >= nalines)
			gw_width = 0;

		np::FloatArray2 gw_removed(nradius, nalines - gw_width);
		mask = np::Uint8Array2(nradius, nalines);
		ippsSet_8u(1, mask, mask.length());

		int k = 0;
		for (int i = 0; i < nalines; i++)
		{
			bool is_gw_aline = false;
			if (gw_region_left.size() == gw_region_right.size())
			{
				for (int j = 0; j < gw_region_left.size(); j++)
				{
					if ((gw_region_left.at(j) >= 0) && (gw_region_right.at(j) < nalines))
					{
						if ((gw_region_left.at(j) <= i) && (gw_region_right.at(j) >= i))
							is_gw_aline = true;
					}
					else if (gw_region_left.at(j) < 0)
					{
						if ((gw_region_left.at(j) + nalines <= i) || (gw_region_right.at(j) >= i))
							is_gw_aline = true;
					}
					else if (gw_region_right.at(j) >= nalines)
					{
						if ((gw_region_left.at(j) <= i) || (gw_region_right.at(j) - nalines >= i))
							is_gw_aline = true;
					}
				}
			}
			
			if (!is_gw_aline)
			{
				if (k == gw_removed.size(1))
					break;
				memcpy(&gw_removed(0, k++), &preprop(0, i), sizeof(float) * nradius);
			}
			else
				ippsSet_8u(0, &mask(0, i), mask.size(0));
		}

		/****************************************************************/
		if (data_save)
		{
			std::ofstream fout;
			fout.open("gw_removed.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)gw_removed.raw_ptr(), sizeof(float) * gw_removed.length());
				fout.close();
			}
		}
		{
			std::ofstream fout;
			fout.open("mask.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)mask.raw_ptr(), sizeof(uint8_t) * mask.length());
				fout.close();
			}
		}
		/****************************************************************/

		// Find cath line
		np::FloatArray std_profile(nradius);
		for (int i = 0; i < nradius; i++)
		{
			np::FloatArray depth0(gw_removed.size(1));
			ippiCopy_32f_C1R(&gw_removed(i, 0), sizeof(float) * nradius, depth0, sizeof(float), { 1, gw_removed.size(1) });
			ippsStdDev_32f(depth0, depth0.length(), &std_profile(i), ippAlgHintFast);
		}

		/****************************************************************/
		if (data_save)
		{
			std::ofstream fout;
			fout.open("std_profile.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)std_profile.raw_ptr(), sizeof(float) * std_profile.length());
				fout.close();
			}
		}
		/****************************************************************/

		int cath_pos = outer_sheath;
		for (int i = outer_sheath; i > (int)(outer_sheath * 0.8); i--)
		{
			if (std_profile(i) < std_profile(i - 1))
			{
				cath_pos = i;
				break;
			}
		}

		cath_removed = np::FloatArray2(nradius - cath_pos - inner_offset, gw_removed.size(1));
		ippiCopy_32f_C1R(&gw_removed(cath_pos, 0), sizeof(float) * gw_removed.size(0),
			&cath_removed(0, 0), sizeof(float) * cath_removed.size(0),
			{ cath_removed.size(0), cath_removed.size(1) });

		ippiSet_8u_C1R(0, &mask(0, 0), sizeof(uint8_t) * mask.size(0), { cath_pos, nalines });
		ippiSet_8u_C1R(0, &mask(nradius - inner_offset - 1, 0), sizeof(uint8_t) * mask.size(0), { inner_offset, nalines });

		/****************************************************************/
		if (data_save)
		{
			std::ofstream fout;
			fout.open("cath_removed.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)cath_removed.raw_ptr(), sizeof(float) * cath_removed.length());
				fout.close();
			}
		}
		if (data_save)
		{
			std::ofstream fout;
			fout.open("mask1.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)mask.raw_ptr(), sizeof(uint8_t) * mask.length());
				fout.close();
			}
		}
		/****************************************************************/
	}

	void delineate_contour(np::FloatArray& contour)
	{
		// Get optimized threshold value by Otsu's method
		cv::Mat _cath_removed = cv::Mat(cath_removed.size(1), cath_removed.size(0), CV_32FC1, cath_removed.raw_ptr());
		_cath_removed.convertTo(_cath_removed, CV_8UC1);
		cv::Mat binary;
		double th_val = cv::threshold(_cath_removed, binary, 127, 255, THRESH_OTSU);

		cv::Mat _preprop = cv::Mat(preprop.size(1), preprop.size(0), CV_32FC1, preprop.raw_ptr());
		_preprop.convertTo(_preprop, CV_8UC1);
		cv::threshold(_preprop, binary, th_val, 255, THRESH_BINARY);

		/****************************************************************/
		if (data_save)
		{
			std::ofstream fout;
			fout.open("binary.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)binary.data, sizeof(uint8_t) * binary.rows * binary.cols);
				fout.close();
			}
		}
		/****************************************************************/

		// Guide-wire & catheter artifact masking
		ippsAnd_8u_I(mask, binary.data, mask.length());

		// Morphological opening
		cv::Mat opened;
		cv::morphologyEx(binary, opened, MORPH_OPEN, getStructuringElement(MORPH_ELLIPSE, cv::Size(9, 9)), cv::Point(-1, -1), 1, BORDER_REFLECT);

		/****************************************************************/
		if (data_save)
		{
			std::ofstream fout;
			fout.open("opened.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)opened.data, sizeof(uint8_t) * opened.rows * opened.cols);
				fout.close();
			}
		}
		/****************************************************************/

		// Area size specification
		cv::Mat labeled, stats, centroids;
		cv::connectedComponentsWithStats(opened, labeled, stats, centroids);

		/****************************************************************/
		if (data_save)
		{
			std::ofstream fout;
			fout.open("labeled.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)labeled.data, sizeof(int) * labeled.rows * opened.cols);
				fout.close();
			}
		}
		/****************************************************************/
		
		// Get gradient image
		np::FloatArray2 opened_32f(nradius, nalines), gradient(nradius, nalines);
		memset(gradient, 0, sizeof(float) * gradient.length());
		ippsConvert_8u32f(opened.data, opened_32f, opened_32f.length());
		ippiSub_32f_C1R(&opened_32f(1, 0), sizeof(float) * (nradius - 1), &opened_32f(0, 0), sizeof(float) * (nradius - 1),
			&gradient(0, 0), sizeof(float) * (nradius - 1), { nradius - 1, nalines });

		/****************************************************************/
		if (data_save)
		{
			std::ofstream fout;
			fout.open("gradient.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)gradient.raw_ptr(), sizeof(float) * gradient.length());
				fout.close();
			}
		}
		/****************************************************************/

		// Delineate lumen contour
		std::vector<float> cx0, cy0;
		for (int i = 0; i < nalines; i++)
		{
			std::vector<int> surface;
			for (int j = 0; j < nradius - 1; j++)
				if (gradient(j, i) == -1.0f)
					surface.push_back(j);

			// Eliminate small component contour
			if (surface.size() == 1)
			{
				int idx = labeled.at<int>(i, surface.at(0) + 1);
				int area = stats.at<int>(idx, CC_STAT_AREA);

				if (area > 250)
				{
					cx0.push_back(i);
					cy0.push_back((float)surface.at(0));
				}
			}
			else if (surface.size() > 1)
			{
				int largest_idx = 0; int area0 = 0;
				for (int j = 0; j < surface.size(); j++)
				{
					int idx = labeled.at<int>(i, surface.at(j) + 1);
					int area = stats.at<int>(idx, CC_STAT_AREA);
					if (area > area0)
					{
						largest_idx = j;
						area0 = area;
					}
				}

				if (area0 > 250)
				{
					cx0.push_back(i);
					cy0.push_back((float)surface.at(largest_idx));
				}
			}
		}
		
		// Interpolation
		int n0 = cx0.size();
		for (int i = 0; i < n0; i++)
		{
			cx0.push_back(cx0.at(i) + nalines);
			cy0.push_back(cy0.at(i));
		}
		np::FloatArray cx1(2 * nalines), cy1(2 * nalines);
		bf::LineSpace_32f(0, 2 * nalines - 1, 2 * nalines, cx1);
		
		if (cx0.size() > 0)
			bf::Interpolation_32f(&cx0[0], &cy0[0], cx1, cy0.size(), cy1.length(), cy1);

		//np::FloatArray sm_contour(2 * nalines);
		//memcpy(sm_contour, cy1, sizeof(float) * cy1.length());
		//for (int i = 1; i < cy1.length() - 1; i++)
		//	ippsMean_32f(&cy1[i - 1], 3, &sm_contour[i], ippAlgHintAccurate);
				
		if (contour.length() == 0)
			contour = np::FloatArray(nalines);		
		memcpy(contour, cy1 + nalines, sizeof(float) * contour.length() / 2);
		memcpy(contour + nalines / 2, cy1 + nalines / 2, sizeof(float) * contour.length() / 2);

		/****************************************************************/
		if (data_save)
		{
			std::ofstream fout;
			fout.open("contour.data", std::ios::out | std::ios::binary);

			if (fout.is_open()) {
				fout.write((const char*)contour.raw_ptr(), sizeof(float) * contour.length());
				fout.close();
			}
		}
		/****************************************************************/
	}

	int _find_fwhm(np::FloatArray& profile, int peak)
	{
		float bg0;
		ippsMin_32f(profile, profile.length(), &bg0);

		int w_r = 0, w_l = 0;

		int i;
		for (i = peak; i < profile.length(); i++)
			if (profile(i) < (profile(peak) + bg0) * 0.75)
				break;
		w_r = i;

		for (i = peak; i >= 0; i--)
			if (profile(i) < (profile(peak) + bg0) * 0.75)
				break;
		w_l = i;

		return w_r - w_l + 1;
	}

private:	
	// Size parameters
	int nalines;
	int nradius;

	// Detection parameters
	double bg, mean;

	bool ref_sup;
	int ref_dist;
	float ref_level;

	int outer_sheath;
	int inner_offset;

	int gw_th;
	float gwp_th;
	int max_w_th;

	// Visualization parameter
	bool data_save;

	// Image buffers
	np::FloatArray2 preprop, cath_removed;
	np::Uint8Array2 mask;
};

#endif // LUMEN_DETECTION_H
