#ifndef CROP
#define CROP

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>

using namespace cv;
using namespace std;

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

	void split();
	void merge();


	void rotate();
};


#endif // !CROP
