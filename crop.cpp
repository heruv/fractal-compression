#include "crop.h"

Crop::Crop(string path_to_img, int domain_size, int rang_size)
{
    input_img_ = imread(path_to_img, IMREAD_GRAYSCALE);

    if (input_img_.empty())
    {
        cout << "Unable to read image" << std::endl;
        abort();
    }

    domain_bloc_size_ = domain_size;
    rang_bloc_size_ = rang_size;

    int new_rows = input_img_.rows - (input_img_.rows % domain_bloc_size_);
    int new_cols = input_img_.cols - (input_img_.cols % domain_bloc_size_);

    input_img_ = input_img_(Rect(0, 0, new_cols, new_rows));
    output_img_.create(new_rows, new_cols, CV_8UC1);

    split();

    merge();

    imshow("merge", output_img_);
    waitKey(0);
}

void Crop::split()
{
    for (int i = 0; i < input_img_.rows; i += rang_bloc_size_)
    {
        for (int j = 0; j < input_img_.cols; j += rang_bloc_size_)
        {
            Mat block = input_img_(Rect(j, i, rang_bloc_size_, rang_bloc_size_));
            rois_.push_back(block);
        }
    }

    for (int i = 0; i < input_img_.rows; i += domain_bloc_size_)
    {
        for (int j = 0; j < input_img_.cols; j += domain_bloc_size_)
        {
            Mat block = input_img_(Rect(j, i, domain_bloc_size_, domain_bloc_size_));
            domain_rois_.push_back(block);
        }
    }
}

void Crop::merge()
{
    int n = 0;
    // so far, we are merging image into new one

    for (int i = 0; i < output_img_.rows; i += rang_bloc_size_)
    {
        for (int j = 0; j < output_img_.cols; j += rang_bloc_size_)
        {
            rois_.at(n).copyTo(output_img_(Rect(j, i, rang_bloc_size_, rang_bloc_size_)));
            n++;
        }
    }
}

Mat Crop::contractive(Mat&block, tuple<float,float> coef, float angle = 90.0f )
{
    Mat reduce = Crop::reduceDomain(block);
    Mat flip   = Crop::flip(reduce, 1);
    Mat rotate = Crop::rotate(flip, angle);

    Mat contractive = get<0>(coef) * rotate + get<1>(coef);

    return contractive;
}

Mat Crop::reduceDomain(Mat&to_reduce)
{
        Mat reduce;
        resize(to_reduce, reduce, rois_[0].size());
        
        return reduce;
}

Mat Crop::rotate(Mat&to_rotate, float angle)
{
    Mat rotate;
    Mat rotated= getRotationMatrix2D(Point((rois_.at(0).cols -1)/ 2.0f, (rois_.at(0).rows -1)/ 2.0f), angle, 1.0);
    warpAffine(to_rotate, rotate, rotated, rois_.at(0).size());

    return rotate;
}

Mat Crop::flip(Mat&to_flip, int direction)
{
    Mat flipped;
    cv::flip(to_flip, flipped, 1);

    return flipped;
}

tuple<float,float> Crop::findContrastBrightness(Mat&domain_block, Mat&range_block)
{
    float sum_D = 0, sum_R = 0, sum_D2 = 0, sum_DR = 0;


    for (int i = 0; i < domain_block.rows; i++)
    {
        for (int j = 0; j < domain_block.cols; j++)
        {
            float D = domain_block.at<uchar>(i, j);
            float R = range_block.at<uchar>(i, j);

            sum_D += D;
            sum_R += R;
            sum_D2 = D * D;
            sum_DR = D * R;
        }
    }

    float contrast = (domain_block.rows * domain_block.cols * sum_DR - sum_D * sum_R) /
        domain_block.rows * domain_block.cols * sum_D2 - sum_D * sum_D;
    
    float brightness = (sum_R - contrast * sum_D) / domain_block.rows * domain_block.cols;


    return {contrast, brightness};
}

void Crop::findBestRatio()
{
    int angle;

    for (int i = 0; i < rois_.size(); i++)
    {
        float error = INFINITY;

        for (int j = 0; j < domain_rois_.size(); j++)
        {
            tuple<float, float> factor = findContrastBrightness(domain_rois_[j], rois_[i]);

            domain_rois_[j] = contractive(domain_rois_[j], factor, angle * 90.0f);

            if (error < findError(domain_rois_[j], rois_[i]))
            {
                error = findError(domain_rois_[j], rois_[i]);

                //it's temp solution
                ratio_.direction = 1;

                ratio_.factor = factor;
                ratio_.rotation_angle = angle * 90.0f;
                ratio_.index = j;
            }
        }
    }
}

float Crop::findError(Mat& domain_block, Mat& range_block)
{

    return 0.0f;
}
