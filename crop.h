#ifndef CROP
#define CROP

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>

using namespace cv;
using namespace std;

struct BestTransform
{
	unsigned int index;
	short direction;
	tuple<float, float> factor;
	float rotation_angle;
};

class Crop
{
public:
	Crop(string path_to_img, int domain_size, int rang_size);

private:
	vector<Mat> rois_;
	vector<Mat> domain_rois_;

	Mat input_img_;
	Mat output_img_;

	int domain_bloc_size_;
	int rang_bloc_size_;

	BestTransform ratio_;

	void split();
	void merge();

	Mat contractive(Mat&block, tuple<float,float> coef, float angle);
	Mat reduceDomain(Mat&to_reduce);
	Mat rotate(Mat&to_rotate, float angle);
	Mat flip(Mat&to_flip, int direction);

	tuple<float,float> findContrastBrightness(Mat&domain_block, Mat&range_block);
	void findBestRatio();
	float findError(Mat& domain_block, Mat& range_block);
};


#endif // !CROP
