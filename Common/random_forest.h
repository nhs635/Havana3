#ifndef RANDOM_FOREST_H
#define RANDOM_FOREST_H

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/ml.hpp>

#include <Common/array.h>

#include <iostream>
#include <fstream>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>


using namespace std;
using namespace cv;
using namespace cv::ml;

enum forest_method {
    CLASSIFICATION = 0,
    REGRESSION = 1
};

class RandomForest
{
public:
    explicit RandomForest()
    {
    }

    ~RandomForest()
    {
    }

public:
    void createForest(int _n_trees, int _n_features, int _n_cats, forest_method _method)
    {
        // Create a random forest model
        forest = RTrees::create();
        method = _method;
		n_trees = _n_trees;
        n_features = _n_features;
		n_cats = _n_cats;

        // Set model parameters
        //forest->setActiveVarCount(0);
        forest->setCalculateVarImportance(true);
		if (_method == CLASSIFICATION)
		{
			forest->setMaxCategories(_n_cats);						
			compo_cmap = (Mat_<float>(_n_cats, 3) << 132, 192, 111, //    67, 191, 220, // normal
													100, 146, 84, // fibrous
													// 128, 128, 128, // calcification
													213, 213, 43, // HLF														
													255, 71, 72, // focal mac
													120, 0, 5); // , // TCFA 
													 //112,  48, 160); // short lifetime
		}

		/*colorc = [[0 0 255]; % 1 : Normal(0x43bfdc)
			[0 255  0]; % 2 : Fibrous(0x649254)
			[255 255 0]; % 3 : Loose fibrous(0xbf9000)
			[255  0  0]; % 5 : Focal mac(0xff4746)
			[255   0   255]; ]; % 6 : TCFA(0x780005)*/

        //forest->setMaxDepth(8);
        //forest->setMinSampleCount(10);
        forest->setRegressionAccuracy(0.0001f);
        forest->setUseSurrogates(false /* true */);        
        //forest->setCVFolds(0 /*10*/); // nonzero causes core dump
        forest->setUse1SERule(true);
        forest->setTruncatePrunedTree(true);
        // dtree->setPriors( priors );
        //forest->setPriors(cv::Mat());

        forest->setTermCriteria(TermCriteria(TermCriteria::MAX_ITER, _n_trees, 0.0001));
    }

    bool train(const char* filename)
    {
		std::ifstream file(filename);
		if (!file)
			return false;
		file.close();

        // Read data from CSV file
        char varTypeSpec[256];
        if (method == CLASSIFICATION)
            sprintf(varTypeSpec, "cat[0]ord[1-%d]", n_features);
        else if (method == REGRESSION)
            sprintf(varTypeSpec, "ord[0-%d]", n_features);

        cv::Ptr<TrainData> dataset = TrainData::loadFromCSV(filename, 0, 0, 1, varTypeSpec);

        int n_samples = dataset->getNSamples();
        if (n_samples == 0)
        {
            printf("Error reading file.\n");
            return false;
        }
        printf("Read %d samples from %s.\n", n_samples, filename);

        // Split dataset into train & test set
        dataset->setTrainTestSplitRatio(0.8, true);
        int n_train_samples = dataset->getNTrainSamples();
        int n_test_samples = dataset->getNTestSamples();
        printf("Training/Test samples: %d / %d\n", n_train_samples, n_test_samples);

        // Training
        printf("Start training...\n");
        forest->train(dataset);
        printf("Successful training...\n");

        // Output attribute importance score
        Mat var_importance = forest->getVarImportance();
        if (!var_importance.empty())
        {
            double rt_imp_sum = sum(var_importance)[0];
            printf("var#\timportance(%%):\n");
            int n = (int)var_importance.total(); // Total number of matrix elements
            for (int i = 0; i < n; i++)
            {
                printf("%-2d\t%-4.1f\n", i, 100.f * var_importance.at<float>(i) / rt_imp_sum);
            }
        }

        // Test
        cv::Mat results_train, results_test;
        float forest_train_error = forest->calcError(dataset, false, results_train);
        float forest_test_error = forest->calcError(dataset, true, results_test);

        // Statistical output results
        if (method == CLASSIFICATION)
        {
            int t = 0, f = 0, total = 0;
            cv::Mat expected_responses_forest = dataset->getTestResponses();

            // Get test set label
            for (int i = 0; i < dataset->getNTestSamples(); i++)
            {
                int responses = (int)results_test.at<float>(i, 0);
                int expected = (int)expected_responses_forest.at<float>(i, 0);
                if (responses == expected)
                    t++;
                else
                    f++;
                total++;
            }

            cout << "Correct answer: " << t << endl;
            cout << "Wrong answer: " << f << endl;
            cout << "Number of test samples: " << total << endl;
            cout << "Training dataset error: " << forest_train_error << "%" << endl;
            cout << "Test dataset error: " << forest_test_error << "%" << endl;
        }
        else if (method == REGRESSION)
        {
            float total = 0;
            cv::Mat expected_responses_forest = dataset->getTestResponses();

            // Get test set value
            for (int i = 0; i < dataset->getNTestSamples(); i++)
            {
                float responses = results_test.at<float>(i, 0);
                float expected = expected_responses_forest.at<float>(i, 0);
                total += abs(responses - expected);
            }
            total /= dataset->getNTestSamples();

            cout << "Averaged error: " << total << endl;
            cout << "Training dataset error: " << forest_train_error << endl;
            cout << "Test dataset error: " << forest_test_error << endl;
        }

		return true;
    }

    void predict(np::FloatArray2& input, np::FloatArray2& output, np::FloatArray2& posterior = np::FloatArray2())
    {
        Mat input_mat(input.size(1), input.size(0), CV_32F, input.raw_ptr());
		Mat output_mat;
		if (method == CLASSIFICATION)
		{
			//output_mat = Mat(input.size(1), 3, CV_32F);
			//tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)input_mat.rows),
			//	[&](const tbb::blocked_range<size_t>& r) {
			//	for (size_t i = r.begin(); i != r.end(); ++i)
			//	{
			//		int idx = (int)forest->predict(input_mat.row((int)i)) - 1;
			//		output_mat.at<float>((int)i, 0) = compo_cmap.at<float>(idx, 0);
			//		output_mat.at<float>((int)i, 1) = compo_cmap.at<float>(idx, 1);
			//		output_mat.at<float>((int)i, 2) = compo_cmap.at<float>(idx, 2);
			//	}
			//});
			//memcpy(output.raw_ptr(), output_mat.data, sizeof(float) * output.length());

			//for (int i = 0; i < input_mat.rows; i++)
			//{
			//	Mat omat;
			//	forest->getVotes(input_mat.row((int)0), omat, 0);
			//}

			output_mat = Mat(input.size(1), n_cats, CV_32S);
			tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)input_mat.rows),
				[&](const tbb::blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i != r.end(); ++i)
				{
					Mat omat;
					forest->getVotes(input_mat.row(i), omat, 0);
					memcpy((int*)output_mat.data + i * output_mat.cols, (int*)omat.data + omat.cols, sizeof(int) * omat.cols);
				}
			});			
			Mat output_temp;
			output_mat.convertTo(output_temp, CV_32F);
			output_mat = output_temp / (float)n_trees;
			Mat output_mat1 = output_mat * compo_cmap;

			memcpy(output.raw_ptr(), output_mat1.data, sizeof(float) * output.length()); // + output_mat1.cols

			if (posterior.length() > 0)
				memcpy(posterior.raw_ptr(), output_mat.data, sizeof(float) * posterior.length());
		}
		else if (method == REGRESSION)
		{
			output_mat = Mat(input.size(1), 1, CV_32F);
			tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)input_mat.rows),
				[&](const tbb::blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i != r.end(); ++i)
				{
					output_mat.at<float>((int)i) = forest->predict(input_mat.row((int)i));
				}
			});
			memcpy(output.raw_ptr(), output_mat.data, sizeof(float) * output.length());
		}		
    }

    void save(const char* filename)
    {
        forest->save(filename);
    }

    bool load(const char* filename)
    {
		std::ifstream file(filename);
		if (!file)
			return false;
		file.close();

        forest = RTrees::load(filename);
		return true;
    }

private:
    cv::Ptr<RTrees> forest;
	int n_trees;
    int n_features;
	int n_cats;
	cv::Mat compo_cmap;
    forest_method method;
};

#endif // RANDOM_FOREST_H
