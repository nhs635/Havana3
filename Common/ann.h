#ifndef _ANN_H_
#define _ANN_H_

#include "Common/Array.h"

#include <ipps.h>
#include <ippi.h>
#include <ippcore.h>

#include <opencv2/core/core.hpp>

using namespace cv;


class ann
{
public:
	ann()
	{
	};

	ann(int _width, int _height, int _x_node, int _h_node, int _y_node) :
		width(_width), height(_height), wh_len(width * height),
		x_node(_x_node), h_node(_h_node), y_node(_y_node)
	{
		// Load net weights and biases
		cv_sc_map = Mat(x_node, 2, CV_32FC1);
		cv_weight1 = Mat(h_node, x_node, CV_32FC1);
		cv_weight2 = Mat(y_node, h_node, CV_32FC1);
		cv_bias1 = Mat(h_node, wh_len, CV_32FC1);
		cv_bias2 = Mat(y_node, wh_len, CV_32FC1);
		cv::Mat cv_bias1_temp(wh_len, h_node, CV_32FC1);
		cv::Mat cv_bias2_temp(wh_len, y_node, CV_32FC1);
		cv_color_map = Mat(y_node, 3, CV_32FC1);

		// Load net weight & bias data
		QFile file("net.bin");
		if (false == file.open(QFile::ReadOnly))
			printf("[ERROR] Invalid net binary data!\n");
		else
		{
			file.read(reinterpret_cast<char *>(cv_sc_map.data), sizeof(float) * 2 * x_node);
			file.read(reinterpret_cast<char *>(cv_weight1.data), sizeof(float) * h_node * x_node);
			file.read(reinterpret_cast<char *>(cv_bias1_temp.data), sizeof(float) * h_node);
			file.read(reinterpret_cast<char *>(cv_weight2.data), sizeof(float) * y_node * h_node);
			file.read(reinterpret_cast<char *>(cv_bias2_temp.data), sizeof(float) * y_node);
			file.read(reinterpret_cast<char *>(cv_color_map.data), sizeof(float) * y_node * 3);

			file.close();
		}

		for (int i = 1; i < wh_len; i++)
		{
			memcpy(cv_bias1_temp.ptr<float>(i), cv_bias1_temp.ptr<float>(0), sizeof(float) * h_node);
			memcpy(cv_bias2_temp.ptr<float>(i), cv_bias2_temp.ptr<float>(0), sizeof(float) * y_node);
		}
		cv::transpose(cv_bias1_temp, cv_bias1);
		cv::transpose(cv_bias2_temp, cv_bias2);

		// Classification result
		//clf_map = np::Uint8Array2(width, height);
		//memset(clf_map.raw_ptr(), 0, sizeof(uint8_t) * wh_len);
		clf_map = np::Uint8Array2(3 * width, height);
		memset(clf_map.raw_ptr(), 0, sizeof(uint8_t) * 3 * wh_len);
	};

	~ann()
	{
	};

	np::Uint8Array2& GetClfMapPtr()
	{
		return clf_map;
	}

	void operator() (const np::FloatArray2& ch1_lifetime, const np::FloatArray2& ch2_lifetime, const np::FloatArray2& intensity_ratio)
	{
		// Make x-vector using ch1, ch2 lifetime and intensity ratio
		np::FloatArray2 x(wh_len, 3);
		memcpy(&x(0, 0), ch1_lifetime.raw_ptr(), sizeof(float) * wh_len);
		memcpy(&x(0, 1), ch2_lifetime.raw_ptr(), sizeof(float) * wh_len);
		memcpy(&x(0, 2), intensity_ratio.raw_ptr(), sizeof(float) * wh_len);

		// [-1, 1] scaling
		for (int i = 0; i < 3; i++)
		{
			ippsSubC_32f_I(cv_sc_map.at<float>(i, 0), &x(0, i), x.size(0));
			ippsDivC_32f_I(0.5f * (cv_sc_map.at<float>(i, 1) - cv_sc_map.at<float>(i, 0)), &x(0, i), x.size(0));
			ippsSubC_32f_I(1.0f, &x(0, i), x.size(0));
		}

		// Matrix multiplication (input layer -> hidden layer)
		cv::Mat cv_x(x_node, ch1_lifetime.length(), CV_32FC1, x);
		cv::Mat cv_h = cv_weight1 * cv_x + cv_bias1;

		// Transfer function (tanh)		
		for (int i = 0; i < cv_h.rows; i++)
		{
			float* ph = cv_h.ptr<float>(i);
			for (int j = 0; j < cv_h.cols; j++)
				ph[j] = tanhf(ph[j]);
		}

		// Matrix multiplication (hidden layer -> output layer)
		cv::Mat cv_y = cv_weight2 * cv_h + cv_bias2;

		{
			// Transfer function (softmax)
			for (int i = 0; i < cv_y.rows; i++)
			{
				float* py = cv_y.ptr<float>(i);
				for (int j = 0; j < cv_h.cols; j++)
					py[j] = expf(py[j]);
			}

			cv::Mat cv_yt(cv_y.cols, cv_y.rows, CV_32FC1);
			cv::transpose(cv_y, cv_yt); // wh_len x 4 (r x c)

			float temp;
			for (int i = 0; i < cv_yt.rows; i++)
			{
				float* pyt = cv_yt.ptr<float>(i);

				temp = 0;
				for (int j = 0; j < cv_yt.cols; j++)
					temp += pyt[j];
				for (int j = 0; j < cv_yt.cols; j++)
					pyt[j] /= temp;
			}

			// Color transformation
			cv::Mat cv_res = cv_yt * cv_color_map;

			// Transformation for visualization
			ippsConvert_32f8u_Sfs((float*)cv_res.data, clf_map.raw_ptr(), clf_map.length(), ippRndZero, 0);
			ippiMirror_8u_C1IR(clf_map.raw_ptr(), sizeof(uint8_t) * clf_map.size(0), { clf_map.size(0), clf_map.size(1) }, ippAxsHorizontal);
		}
		{
			//// Classification result (find max index)
			//cv::Mat cv_yt(cv_y.cols, cv_y.rows, CV_32FC1);
			//cv::transpose(cv_y, cv_yt);

			//float temp;
			//cv::Mat cv_res(1, cv_yt.rows, CV_32SC1);
			//for (int i = 0; i < cv_res.cols; i++)
			//	ippsMaxIndx_32f(cv_yt.ptr<float>(i), cv_yt.cols, &temp, (int*)cv_res.data + i);
			//
			//// Transformation for visualization
			//np::FloatArray res_temp(cv_res.cols);
			//ippsConvert_32s32f((int*)cv_res.data, res_temp.raw_ptr(), cv_res.cols);
			//ippsConvert_32f8u_Sfs((float*)res_temp.raw_ptr(), clf_map.raw_ptr(), cv_res.cols, ippRndZero, 0);
			//ippsMulC_8u_ISfs(int(260 / y_node), clf_map.raw_ptr(), cv_res.cols, 0);
			//ippiMirror_8u_C1IR(clf_map.raw_ptr(), sizeof(uint8_t) * clf_map.size(0), { clf_map.size(0), clf_map.size(1) }, ippAxsHorizontal);
		}
	}

private:
	// net weights and biases
	int width, height, wh_len;
	int x_node, h_node, y_node;

	Mat cv_sc_map;
	Mat cv_weight1, cv_weight2;
	Mat cv_bias1, cv_bias2;
	Mat cv_color_map;

	np::Uint8Array2 clf_map;
};

#endif
