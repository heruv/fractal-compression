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

Mat Crop::contractive(Mat&block, float brightness = 0.0f, float contrast = 1.0f )
{
    Mat reduce = Crop::reduceDomain(block);
    Mat flip   = Crop::flip(reduce, 1);
    Mat rotate = Crop::rotate(flip);

    Mat contractive = contrast * rotate + brightness;

    return contractive;
}

Mat Crop::reduceDomain(Mat&to_reduce)
{
        const Mat reduce;
        resize(to_reduce, reduce, rois_[0].size(), INTER_AREA);
        
        return reduce;
}

Mat Crop::rotate(Mat&to_rotate)
{
    Mat rotate;
    Mat rotated= getRotationMatrix2D(Point((rois_.at(0).cols -1)/ 2.0f, (rois_.at(0).rows -1)/ 2.0f), 90.0, 1.0);
    warpAffine(to_rotate, rotate, rotated, rois_.at(0).size());

    return rotate;
}

Mat Crop::flip(Mat&to_flip, int direction)
{
    Mat flipped;
    cv::flip(to_flip, flipped, 1);

    return flipped;
}
