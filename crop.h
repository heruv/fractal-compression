#ifndef CROP
#define CROP

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <Windows.h>

using namespace cv;
using namespace std;

struct BestTransform
{
	int domain_index;
	short direction;
	tuple<float, float> factor;
	float rotation_angle;
	Mat block;
};


inline void printBestTransform(const BestTransform& bt) {
    std::cout << "Best Transform Details:\n";
    std::cout << std::string(30, '-') << "\n";

    // Вывод направления (0 - нет, 1 - горизонтальное)
    std::string dir_str = (bt.direction == 1) ? "Horizontal" : "None";
    std::cout << "| " << std::setw(15) << "Flip Direction"
        << " | " << std::setw(10) << dir_str << " |\n";

    // Вывод коэффициентов контраста и яркости
    std::cout << "| " << std::setw(15) << "Contrast"
        << " | " << std::setw(10) << std::get<0>(bt.factor) << " |\n";
    std::cout << "| " << std::setw(15) << "Brightness"
        << " | " << std::setw(10) << std::get<1>(bt.factor) << " |\n";

    // Вывод угла поворота
    std::cout << "| " << std::setw(15) << "Rotation Angle"
        << " | " << std::setw(10) << bt.rotation_angle << " |\n";

    std::cout << std::string(30, '-') << "\n";
}

class Crop
{
public:
	Crop(string path_to_img, int domain_size, int rang_size);

private:
	mutex ratio_mutex;
	mutex cout_mutex;

	vector<Mat> rois_;
	vector<Mat> domain_rois_;

	Mat input_img_;
	Mat output_img_;

	int domain_bloc_size_;
	int rang_bloc_size_;

	vector<BestTransform> total_ratio_;
	vector<BestTransform> main_ratio_;

	void split();
	void merge();

	Mat contractive(Mat&block, float contrast, float brightness, float angle, int direction);
	Mat reduceDomain(Mat&to_reduce);
	Mat rotate(Mat&to_rotate, float angle);
	Mat flip(Mat&to_flip, int direction);

	tuple<float,float> findContrastBrightness(Mat&domain_block, Mat&range_block);
	void makeAllTransforms();
	float findError(Mat& domain_block, Mat& range_block);
	void compress();

	void makeTreads();
};


#endif // !CROP
