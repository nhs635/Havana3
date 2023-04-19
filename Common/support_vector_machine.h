#ifndef SUPPORT_VECTOR_MACHINE_H
#define SUPPORT_VECTOR_MACHINE_H

#include <iostream>
#include <fstream>

#include <stdarg.h>

#include <opencv2/core.hpp>

#include <Common/array.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include "svm.h"


using namespace std;
using namespace cv;


class SupportVectorMachine
{
public:
    explicit SupportVectorMachine() : x_space(nullptr)
    {
		prob.y = nullptr;
		prob.x = nullptr;
    }

    ~SupportVectorMachine()
    {
		for (int i = 0; i < n_cats; i++)
			svm_free_and_destroy_model(&model[i]);
		svm_destroy_param(&param);

		if (prob.l > 0)
		{
			if (prob.y) delete[] prob.y;
			if (prob.x) delete[] prob.x;
			if (x_space) delete[] x_space;
		}
    }

public:
    void createMachine(int _n_features, int _n_cats)
    {		
		// Set model parameter
		n_features = _n_features;
		n_cats = _n_cats;

		param.svm_type = C_SVC;
		param.kernel_type = LINEAR;
		param.degree = 3;
		param.gamma = 0;	// 1/num_features
		param.coef0 = 0;
		param.nu = 0.5;
		param.cache_size = 100;
		param.C = 1; // 1? 
		param.eps = 1e-3;
		param.p = 0.1;
		param.shrinking = 0;
		param.probability = 1;
		param.nr_weight = 0;
		param.weight_label = NULL;
		param.weight = NULL;
		
		// Set array for standardization
		mean = np::DoubleArray(n_features);
		std = np::DoubleArray(n_features);

        // Allocate composition colormap matrix
		compo_cmap = Mat_<float>(_n_cats, 3);
    }

	void setColormap(int len, ...)
	{
		va_list ap;
		va_start(ap, len);
		for (int i = 0; i < len; i++)
		{
			uint32_t rgb = va_arg(ap, uint32_t);
			for (int j = 0; j < 3; j++)
				compo_cmap.at<float>(i, j) = (rgb >> (8 * (2 - j))) & 0xff;
		}
		va_end(ap);
	}
	
	bool train(const char* filename)
	{
		printf("Start training...\n");

		// Read data from CSV file
		double* label;
		std::ifstream file(filename);
		if (file.is_open())
		{
			// Initialize
			prob.l = 0;

			// Check the number of samples
			string line;
			while (std::getline(file, line))
				prob.l++;
			file.clear();
			file.seekg(0, ios::beg);

			// Allocate arrays in problem object
			int n_features1 = n_features + 1;
			label = new double[prob.l];
			prob.y = new double[prob.l];
			prob.x = new svm_node*[prob.l];
			x_space = new svm_node[prob.l * n_features1];
			np::DoubleArray2 feature_vect(prob.l, n_features);

			// Read feature values
			int i = 0;
			while (std::getline(file, line))
			{
				prob.x[i] = &x_space[n_features1 * i];

				stringstream stream(line);
				string tok;

				int j = 0;
				while (std::getline(stream, tok, ','))
				{
					if (j == 0)
					{
						// Label
						label[i] = stod(tok);
					}
					else
					{
						// Features						
						x_space[n_features1 * i + j - 1].index = j;
						x_space[n_features1 * i + j - 1].value = stod(tok);
						feature_vect(i, j - 1) = x_space[n_features1 * i + j - 1].value;
					}
					j++;
				}
				x_space[n_features1 * i + j - 1].index = -1;

				i++;
			}
			file.close();

			// Find mean & std for standardization		
			for (i = 0; i < n_features; i++)
				ippsMeanStdDev_64f(&feature_vect(0, i), feature_vect.size(0), &mean(i), &std(i));

			// Feature standardization
			for (i = 0; i < prob.l; i++)
				for (int j = 0; j < n_features; j++)
					prob.x[i][j].value = (prob.x[i][j].value - mean(j)) / std(j);
		}
		else
		{
			printf("Failed to load training dataset...\n");
			return false;
		}

		// Training each one vs. all classifier model
		for (int index = 0; index < n_cats; index++)
		{
			// Set positive & negative label
			for (int i = 0; i < prob.l; i++)
			{
				if (int(label[i]) == (index + 1))
					prob.y[i] = +1.0;
				else
					prob.y[i] = -1.0;
			}

			// Train SVM model
			const char *error_msg = svm_check_parameter(&prob, &param);
			if (error_msg != NULL)
			{
				printf("Failed to train: %s\n", error_msg);
				return false;
			}
			model[index] = svm_train(&prob, &param);

			// Find train error
			int correct = 0;
			for (int i = 0; i < prob.l; i++)
			{
				double predicted_label = svm_predict(model[index], prob.x[i]);
				if (predicted_label == prob.y[i])
					correct++;
			}
			printf("\n\n** Train accuracy: %.4f\n", (double)correct / (double)prob.l);
		}

		// Memory disallocation					
		delete[] label;

		return true;
	}
	
    void predict(np::FloatArray2& input, np::FloatArray2& softmax_prob, np::FloatArray2& logistics_prob)
    {		
		np::DoubleArray2 svm_scores(n_cats, input.size(1));		
		np::DoubleArray2 softmax_temp(n_cats, input.size(1));
		np::DoubleArray2 logistics_temp(n_cats, input.size(1));
						
		// SVM prediction
		tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)input.size(1)),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
				svm_node* x_space = new svm_node[n_features + 1];
				for (int j = 0; j < n_features; j++)
				{
					x_space[j].index = j + 1;
					x_space[j].value = (input(j, (int)i) - mean(j)) / std(j); // Feature standardization
				}
				x_space[n_features].index = -1;

				for (int c = 0; c < n_cats; c++)
				{
					// Logistic probaility (sigmoid function)				
					double prob_estimates[2];
					svm_predict_probability(model[c], x_space, prob_estimates, &svm_scores(c, (int)i));
					logistics_temp(c, (int)i) = prob_estimates[0];
				}

				delete[] x_space;
			}
		});
		ippsConvert_64f32f(logistics_temp, logistics_prob, logistics_prob.length());
						
		// Softmax probability
		ippsExp_64f(svm_scores, softmax_temp, softmax_temp.length());

		tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)input.size(1)),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
				double sum;
				ippsSum_64f(&softmax_temp(0, (int)i), softmax_temp.size(0), &sum);
				if (!isnan(sum))
					ippsDivC_64f_I(sum, &softmax_temp(0, (int)i), softmax_temp.size(0));
				else
					ippsSet_64f(1.0 / (double)n_cats, &softmax_temp(0, (int)i), softmax_temp.size(0));
			}
		});
		ippsConvert_64f32f(softmax_temp, softmax_prob, softmax_prob.length());
    }

	void pseudocolor(np::FloatArray2& softmax_prob, np::FloatArray2& softmax_map,
		np::FloatArray2& logistics_prob, np::FloatArray2& logistics_map, bool logistic_normalize = true)
	{
		// Pseudo-color composition map (softmax posterior)
		{
			Mat post_prob(softmax_prob.length() / n_cats, n_cats, CV_32F, softmax_prob.raw_ptr());
			Mat compo_map = post_prob * compo_cmap;
			memcpy(softmax_map.raw_ptr(), compo_map.data, sizeof(float) * softmax_map.length());
		}

		// Pseudo-color composition map (logistics posterior)
		{
			Mat logit_prob(logistics_prob.length() / n_cats, n_cats, CV_32F, logistics_prob.raw_ptr());

			// Normalize logitstic probability
			np::FloatArray2 logit_prob0(logit_prob.cols, logit_prob.rows);
			memcpy(logit_prob0.raw_ptr(), logit_prob.data, sizeof(float) * logistics_prob.length());
			if (logistic_normalize)
			{
				tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)logit_prob.rows),
					[&](const tbb::blocked_range<size_t>& r) {
					for (size_t i = r.begin(); i != r.end(); ++i)
					{
						float sum;
						ippsSum_32f(&logit_prob0(0, (int)i), logit_prob0.size(0), &sum, ippAlgHintFast);
						///if (!logistic_normalize)
						///{
						///	if (sum > 1.0f)
						///		ippsDivC_32f_I(sum, &logit_prob0(0, (int)i), logit_prob0.size(0));
						///}
						///else
						ippsDivC_32f_I(sum, &logit_prob0(0, (int)i), logit_prob0.size(0));
					}
				});
			}

			Mat logit_prob1(logit_prob.rows, logit_prob.cols, CV_32F, logit_prob0.raw_ptr());
			Mat compo_map = logit_prob1 * compo_cmap;
			memcpy(logistics_map.raw_ptr(), compo_map.data, sizeof(float) * logistics_map.length());
		}
	}

    void save(const char* filename)
    {
		for (int i = 0; i < n_cats; i++)
		{
			char filename1[256];
			sprintf(filename1, "%s_%d.xml", filename, i + 1);
			svm_save_model(filename1, model[i]);
		}

		char filename1[256];
		sprintf(filename1, "%s_standardize.ms", filename);

		std::ofstream fout;
		fout.open(filename1, std::ios::out | std::ios::binary);

		if (fout.is_open()) {
			fout.write((const char*)mean.raw_ptr(), sizeof(double) * mean.length());
			fout.write((const char*)std.raw_ptr(), sizeof(double) * std.length());
			fout.close();
		}
    }

    bool load(const char* filename)
    {
		for (int i = 0; i < n_cats; i++)
		{
			char filename1[256];
			sprintf(filename1, "%s_%d.xml", filename, i + 1);

			std::ifstream file(filename1);
			if (!file)
				return false;
			file.close();

			model[i] = svm_load_model(filename1);
		}

		char filename1[256];
		sprintf(filename1, "%s_standardize.ms", filename);

		std::ifstream fin;
		fin.open(filename1, std::ios::in | std::ios::binary);

		if (fin.is_open()) {
			fin.read((char*)mean.raw_ptr(), sizeof(double) * mean.length());
			fin.read((char*)std.raw_ptr(), sizeof(double) * std.length());
			fin.close();
		}
		
		return true;
    }

private:
	svm_model *model[10];
	svm_parameter param;
	svm_problem prob;
	svm_node *x_space;
	
	np::DoubleArray mean, std;

    int n_features;
	int n_cats;

	cv::Mat compo_cmap;
};

#endif // SUPPORT_VECTOR_MACHINE_H
