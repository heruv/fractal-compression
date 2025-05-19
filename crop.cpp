#include "crop.h"
#include <random>

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


    split();

    makeAllTransforms();

    compress();
    merge();

    imshow("input image", input_img_);
    waitKey(0);
    imshow("total image", output_img_);
    imwrite("out.jpg", output_img_);
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
    Mat image(input_img_.size(), CV_8UC1, Scalar(128));

    for (int iter = 0; iter < 15; iter++)
    {
        Mat new_img = image.clone();
        vector<Mat> domain_rois;

        for (int y = 0; y < new_img.rows; y += domain_bloc_size_)
        {
            for (int x = 0; x < new_img.cols; x += domain_bloc_size_)
            {
                Mat block = new_img(Rect(x, y, domain_bloc_size_, domain_bloc_size_));
                domain_rois.push_back(block);
            }
        }

        for (int i = 0; i < main_ratio_.size(); i++)
        {
            BestTransform ratio = main_ratio_[i];

            Mat domain_block = domain_rois[ratio.domain_index]; 

            Mat transformed = contractive(domain_block, get<0>(ratio.factor), get<1>(ratio.factor), ratio.rotation_angle, ratio.direction);

            Mat reduced;
            resize(transformed, reduced, Size(rang_bloc_size_, rang_bloc_size_));

            int y = (i * rang_bloc_size_) / image.cols * rang_bloc_size_;
            int x = (i * rang_bloc_size_) % image.cols;
            reduced.copyTo(new_img(Rect(x, y, rang_bloc_size_, rang_bloc_size_)));
        }

        image = new_img;
    }

    output_img_ = image.clone();
}

Mat Crop::contractive(Mat&block, float contrast = 1.0f, float brightness = 0.0f, float angle = 90.0f, int direction = 1 )
{
    Mat result(4, 4 ,CV_8UC1);
    Mat flip   = Crop::flip(block, direction);
    Mat rotate = Crop::rotate(flip, angle);

    rotate.convertTo(result, CV_8UC1, contrast, brightness);

    return result;
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
    if (!domain_block.isContinuous()) {
        domain_block = domain_block.clone();
    }

    if (!range_block.isContinuous()) {
        range_block = range_block.clone();
    }

    Mat domain_reshaped = domain_block.reshape(1, domain_block.total());
    Mat range_reshaped = range_block.reshape(1, range_block.total());

    Mat domain_reshaped_float, range_reshaped_float;
    domain_reshaped.convertTo(domain_reshaped_float, CV_32F);
    range_reshaped.convertTo(range_reshaped_float, CV_32F);

    Mat A(domain_reshaped.rows, 2, CV_32F);
    domain_reshaped_float.copyTo(A.col(0));
    A.col(1) = Mat::ones(domain_reshaped.rows, 1, CV_32F);


    // Solve the linear system
    Mat x;
    solve(A, range_reshaped_float, x, DECOMP_SVD);

    return { x.at<float>(0), x.at<float>(1) };
}


void Crop::makeAllTransforms()
{
    BestTransform ratio;

    for (int direction = 0; direction < 2; direction++)
    {
        for (int index = 0; index < 4; index++)
        {
            for (int i = 0; i < domain_rois_.size(); i++)
            {
                Mat temp = reduceDomain(domain_rois_[i]);
                Mat contractive = Crop::contractive(temp, 1.0f, 0.0f, index * 90.0f, direction);

                ratio.block = contractive;
                ratio.direction = direction;
                ratio.rotation_angle = index * 90.0f;
                ratio.domain_index = i;

                total_ratio_.push_back(ratio);
            }
        }
    }
}

float Crop::findError(Mat& domain_block, Mat& range_block)
{
    CV_Assert(domain_block.size() == range_block.size());
    CV_Assert(domain_block.type() == range_block.type());

    Mat diff;
    absdiff(domain_block, range_block, diff);
    diff.convertTo(diff, CV_32F);
    return sum(diff.mul(diff))[0];  // Sum of squared differences
}

void Crop::compress()
{
    for (auto& i : rois_)
    {
        float min_error = INFINITE;
        BestTransform best;

        for (auto& domain_data : total_ratio_)
        {
            auto [contrast, brightness] = findContrastBrightness(domain_data.block, i);
            Mat transformed = contractive(domain_data.block, contrast, brightness, domain_data.rotation_angle, domain_data.direction);

            float error = findError(transformed, i);

            if (error < min_error) 
            {
                min_error = error;
                best = domain_data;
                best.factor = { contrast, brightness };
            }
        }

        main_ratio_.push_back(best);

    }
}

//void Crop::makeTreads()
//{
//    int area_size = rois_.size() / 8;
//
//    vector<thread> thr;
//
//    for (int i = 0, start_position = 0; i < 8; i++, start_position+=area_size)
//    {
//        thr.emplace_back(&Crop::compress, this, area_size, start_position);
//    }
//
//    for (auto& t : thr)
//    {
//        t.join();
//    }
//
//}
